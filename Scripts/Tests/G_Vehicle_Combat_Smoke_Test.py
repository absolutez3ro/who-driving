"""Listen server + two actual owned clients. Commands execute through latent client RPCs."""
import importlib.util
import json
from pathlib import Path
import time
import traceback
import unreal

ROOT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).parent
spec = importlib.util.spec_from_file_location('seat_smoke_helper', ROOT / 'Scripts/Tests/G_Vehicle_Seating_Smoke_Test.py')
helper = importlib.util.module_from_spec(spec)
spec.loader.exec_module(helper)
state = helper.testState
report = ROOT / 'Whos_Driving/Saved/M8Integration20261016/network_result.json'

def Combat(controller):
    return controller.get_driver_character().get_combat_component()

def Weapon(controller, primary=False):
    return Combat(controller).get_weapon(unreal.GWeaponSlot.PRIMARY if primary else unreal.GWeaponSlot.SECONDARY)

def Phase(name):
    helper.SetPhase(name)

def Check(message, condition=True):
    helper.Check(message, condition)

def Command(controller, function, delay=0.15):
    state['action_uuid'] = state.get('action_uuid', 83000) + 1
    info = unreal.LatentActionInfo()
    info.import_text('(Linkage=0,UUID=' + str(state['action_uuid']) + ',ExecutionFunction="' + function + '",CallbackTarget="' + controller.get_path_name() + '")')
    unreal.get_default_object(unreal.SystemLibrary).call_method('Delay', args=(controller, delay, info))

def AimAt(controller, point):
    avatar = controller.get_driver_character()
    camera = avatar.get_component_by_class(unreal.CameraComponent)
    desired = (point - camera.get_world_location()).rotator()
    current = camera.get_world_rotation()
    sensitivity = avatar.get_editor_property('CameraSensitivity')
    avatar.do_aim(((desired.yaw - current.yaw + 180) % 360 - 180) / sensitivity, (desired.pitch - current.pitch) / sensitivity)

def Screenshot(controller, name):
    if not (report.parent / 'capture_visuals.flag').exists():
        return
    path = report.parent / (name + '.png')
    unreal.SystemLibrary.execute_console_command(controller.get_outer().get_outer(),
        'HighResShot 1280x720 filename="' + path.as_posix() + '"', controller)

def Finish(error=None):
    result = {'passed': error is None, 'error': error, 'phase': state.get('phase'), 'checks': state.get('checks'),
              'snapshots': state.get('snapshots', []), 'initial_debug': state.get('initial_debug'),
              'scope': 'listen server + two owned clients; no direct server combat mutation'}
    try:
        helper.SetPlayMode(state['settings'], state['old_mode'])
        for name, value in state['old_settings'].items():
            state['settings'].set_editor_property(name, value)
        state['mode'].set_editor_property('default_pawn_class', state['old_pawn'])
        result['settings_restored'] = True
    except Exception:
        result['restore_error'] = traceback.format_exc()
    if state.get('callback') is not None:
        unreal.unregister_slate_post_tick_callback(state['callback'])
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play()
    report.write_text(json.dumps(result, indent=2))
    state.clear()

def Snapshot():
    result = {'phase': state['phase'], 'players': []}
    for pc in state.get('server_pcs', []):
        combat = Combat(pc)
        result['players'].append({'id': helper.GetPlayerId(pc), 'seat': combat.get_seat_index(), 'outside': combat.is_outside(),
            'alpha': combat.get_window_alpha(), 'selected': str(combat.get_selected_slot()), 'can_fire': combat.can_fire(),
            'pistol': Weapon(pc).get_bullet_count(), 'rifle': Weapon(pc, True).get_bullet_count(),
            'reserve': Weapon(pc, True).get_reserve_ammo(), 'health': combat.get_health(),
            'camera': str(pc.get_driver_character().get_component_by_class(unreal.CameraComponent).get_world_location()),
            'head': str(next(mesh for mesh in pc.get_driver_character().get_components_by_class(unreal.SkeletalMeshComponent) if mesh.get_name() == 'CharacterMesh0').get_socket_location('head')),
            'muzzle': str(Weapon(pc, combat.get_selected_slot() == unreal.GWeaponSlot.PRIMARY).get_third_person_mesh().get_socket_location('MuzzleSocket')),
            'weapon_bounds': str(unreal.SystemLibrary.get_component_bounds(Weapon(pc, combat.get_selected_slot() == unreal.GWeaponSlot.PRIMARY).get_first_person_mesh())),
            'weapon_transform': str(Weapon(pc, combat.get_selected_slot() == unreal.GWeaponSlot.PRIMARY).get_first_person_mesh().get_world_transform()),
            'pawn': pc.get_controlled_pawn().get_path_name() if pc.get_controlled_pawn() else None})
    state.setdefault('snapshots', []).append(result)

def TraceDiagnostics():
    host = state['host']
    victim = state['victim'].get_driver_character()
    mesh = next(part for part in victim.get_components_by_class(unreal.SkeletalMeshComponent) if part.get_name() == 'CharacterMesh0')
    channel = next(getattr(unreal.CollisionChannel, name) for name in dir(unreal.CollisionChannel) if 'WEAPON' in name.upper())
    trace_type = next(getattr(unreal.TraceTypeQuery, name) for name in dir(unreal.TraceTypeQuery) if 'WEAPON' in name.upper())
    origin = host.get_driver_character().get_component_by_class(unreal.CameraComponent).get_world_location()
    target = mesh.get_socket_location('head')
    result = {'view': str(host.get_player_view_point()), 'aim': str(Combat(host).get_aim_direction()), 'target': str(target),
        'boxes': [{'name': box.get_name(), 'location': str(box.get_world_location()), 'enabled': str(box.get_collision_enabled()),
                   'response': str(box.get_collision_response_to_channel(channel))} for box in victim.get_components_by_class(unreal.BoxComponent)]}
    for complex_trace in (False, True):
        hit = unreal.SystemLibrary.line_trace_single(host, origin, target, trace_type, complex_trace,
            [host.get_driver_character()], unreal.DrawDebugTrace.NONE, True)
        result[str(complex_trace)] = hit.export_text() if hasattr(hit, 'export_text') else str(hit)
    (report.parent / 'trace_diagnostics.json').write_text(json.dumps(result, indent=2))

def Tick(delta):
    try:
        now = time.monotonic()
        phase_limit = 180 if state['phase'] == 'initial' or state['phase'].startswith('capture_') else 40
        if now - state['started'] > 600 or now - state['phase_start'] > phase_limit:
            Snapshot()
            raise RuntimeError('Timed out in ' + state['phase'])
        worlds = unreal.EditorLevelLibrary.get_pie_worlds(True)
        servers = [world for world in worlds if unreal.GameplayStatics.get_game_mode(world)]
        clients = [world for world in worlds if not unreal.GameplayStatics.get_game_mode(world)]
        if not servers or len(clients) < 2:
            return
        if state['phase'] == 'initial':
            server = servers[0]
            pcs = list(unreal.GameplayStatics.get_all_actors_of_class(server, unreal.GPlayerController))
            local_clients = [unreal.GameplayStatics.get_player_controller(world, 0) for world in clients]
            state['initial_debug'] = {'worlds': len(worlds), 'server_controllers': len(pcs),
                'clients': [pc.get_path_name() if pc else None for pc in local_clients],
                'server_avatars': [pc.get_driver_character().get_path_name() if pc.get_driver_character() else None for pc in pcs]}
            if len(pcs) != 3 or any(not isinstance(pc, unreal.GPlayerController) or not pc.get_driver_character() or not pc.get_seated_vehicle() for pc in pcs + local_clients):
                return
            if any(not Weapon(pc) or not Weapon(pc, True) for pc in pcs + local_clients):
                return
            host = unreal.GameplayStatics.get_player_controller(server, 0)
            state.update({'server_pcs': pcs, 'host': host, 'clients': local_clients, 'server': server, 'car': host.get_seated_vehicle()})
            for world in worlds:
                for car in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.GPlayerCar):
                    car.get_movement_component().set_component_tick_enabled(False)
                    mesh = next(part for part in car.get_components_by_class(unreal.SkeletalMeshComponent) if part.get_name() == 'VehicleMesh')
                    mesh.set_all_bodies_simulate_physics(False)
                    car.set_actor_location(unreal.Vector(-900, -800, 20), False, True)
                    car.set_actor_rotation(unreal.Rotator(), True)
            Check('Three independent server-owned loadouts', all(Weapon(pc).get_bullet_count() == 15 and Weapon(pc, True).get_bullet_count() == 40 for pc in pcs))
            Check('Driver keeps car possession', host.get_controlled_pawn() == state['car'])
            for pc in pcs:
                character = pc.get_driver_character()
                Check('Manny retained', '/Manny/SKM_Manny_Combat' in character.get_component_by_class(unreal.SkeletalMeshComponent).get_editor_property('skeletal_mesh_asset').get_path_name())
            for pc in [host] + local_clients:
                Command(pc, 'RequestFireStart')
            Phase('inside_fire')
            return
        elapsed = now - state['phase_start']
        host = state['host']
        first, second = state['clients']
        if state['phase'] == 'inside_fire' and elapsed > 0.55:
            for pc in [host, first, second]:
                Command(pc, 'RequestFireStop')
            server_clients = [pc for pc in state['server_pcs'] if pc != host]
            Check('Passenger inside shots rejected on server', all(Weapon(pc).get_bullet_count() == 15 for pc in server_clients))
            Check('Driver secondary fires inside', Weapon(host).get_bullet_count() < 15)
            Snapshot()
            state['car'].get_movement_component().set_throttle_input(0.65)
            for pc in [host, first, second]:
                seat = Combat(pc).get_seat_index()
                Command(pc, 'RequestWindowLeft' if seat in (0, 2) else 'RequestWindowRight')
            Phase('outside')
        elif state['phase'] == 'outside' and elapsed > 1.0:
            if not all(Combat(pc).is_outside() and Combat(pc).get_window_alpha() > 0.99 for pc in state['server_pcs']):
                return
            Check('Q/E sides validated by server for three seats')
            Check('Leaning driver remains vehicle controller', host.get_controlled_pawn() == state['car'])
            Check('Leaning locks steering', state['car'].get_movement_component().is_window_driving_locked())
            Check('Driver preserves last throttle while outside', abs(state['car'].get_movement_component().get_window_throttle() - 0.65) < 0.01)
            for world in clients:
                avatars = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Whos_DrivingCharacter)
                Check('Outside state replicated to observers', len(avatars) == 3 and all(character.get_combat_component().get_window_alpha() > 0.99 for character in avatars))
            for pc in [host, first, second]:
                Command(pc, 'RequestPrimaryWeapon')
            Phase('primary_equipped')
        elif state['phase'] == 'primary_equipped' and elapsed > 1.2:
            Check('Primary selected independently', all(Combat(pc).get_selected_slot() == unreal.GWeaponSlot.PRIMARY for pc in state['server_pcs']))
            if (report.parent / 'capture_visuals.flag').exists():
                Screenshot(first, 'combat_first_person')
                Phase('capture_first_person')
            else:
                Command(first, 'RequestFireStart')
                Phase('outside_fire')
        elif state['phase'] == 'capture_first_person' and elapsed > 1.0:
            Command(first, 'RequestAimStart')
            Phase('aim_view')
        elif state['phase'] == 'aim_view' and elapsed > 0.8:
            Screenshot(first, 'combat_aim')
            Phase('capture_aim')
        elif state['phase'] == 'capture_aim' and elapsed > 1.0:
            Command(first, 'RequestAimStop')
            position = state['car'].get_actor_location() + unreal.Vector(300, -550, 180)
            rotation = (state['car'].get_actor_location() + unreal.Vector(0, 0, 110) - position).rotator()
            transform = unreal.Transform(location=position, rotation=rotation)
            library = unreal.get_default_object(unreal.GameplayStatics)
            camera = library.call_method('BeginDeferredActorSpawnFromClass', args=(second.get_outer().get_outer(), unreal.CameraActor.static_class(), transform,
                unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN, None, unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
            library.call_method('FinishSpawningActor', args=(camera, transform, unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
            state['observer_camera'] = camera
            second.set_view_target_with_blend(camera)
            Phase('observer_view')
        elif state['phase'] == 'observer_view' and elapsed > 0.5:
            Screenshot(second, 'combat_observer')
            Phase('capture_observer')
        elif state['phase'] == 'capture_observer' and elapsed > 1.0:
            second.set_view_target_with_blend(second.get_driver_character())
            Command(first, 'RequestFireStart')
            Phase('outside_fire')
        elif state['phase'] == 'outside_fire' and elapsed > 0.6:
            Command(first, 'RequestFireStop')
            Phase('outside_stop')
        elif state['phase'] == 'outside_stop' and elapsed > 0.6:
            server_first = next(pc for pc in state['server_pcs'] if helper.GetPlayerId(pc) == helper.GetPlayerId(first))
            Check('Remote client fires primary through owned RPC', Weapon(server_first, True).get_bullet_count() < 40)
            Check('Other players retain independent magazines', all(Weapon(pc, True).get_bullet_count() == 40 for pc in state['server_pcs'] if pc != server_first))
            state['spent'] = 40 - Weapon(server_first, True).get_bullet_count()
            Snapshot()
            Command(first, 'RequestReload')
            Phase('reload')
        elif state['phase'] == 'reload' and elapsed > 0.8 and not state.get('reload_captured') and (report.parent / 'capture_visuals.flag').exists():
            Screenshot(first, 'combat_reload')
            state['reload_captured'] = True
        elif state['phase'] == 'reload' and elapsed > 2.7:
            server_first = next(pc for pc in state['server_pcs'] if helper.GetPlayerId(pc) == helper.GetPlayerId(first))
            Check('Reload grants ammunition on server after delay', Weapon(server_first, True).get_bullet_count() == 40)
            Check('Reload subtracts exact personal reserve', Weapon(server_first, True).get_reserve_ammo() == 120 - state['spent'])
            Check('Reload replicated to owner', Weapon(first, True).get_reserve_ammo() == 120 - state['spent'])
            for pc in [host, first, second]:
                seat = Combat(pc).get_seat_index()
                Command(pc, 'RequestWindowRight' if seat in (0, 2) else 'RequestWindowLeft')
            Phase('inside_return')
        elif state['phase'] == 'inside_return' and elapsed > 1.0:
            Check('Driver automatically returns to secondary', Combat(host).get_selected_slot() == unreal.GWeaponSlot.SECONDARY)
            Check('Returning driver regains steering', not state['car'].get_movement_component().is_window_driving_locked())
            Check('Returning driver releases latched throttle', abs(state['car'].get_movement_component().get_throttle_input()) < 0.01)
            Check('Passengers retain their selected primary', all(Combat(pc).get_selected_slot() == unreal.GWeaponSlot.PRIMARY for pc in state['server_pcs'] if pc != host))
            Check('Passengers cannot fire after returning', all(not Combat(pc).can_fire() for pc in state['server_pcs'] if pc != host))
            victim = next(pc for pc in state['server_pcs'] if helper.GetPlayerId(pc) == helper.GetPlayerId(first))
            state['victim'] = victim
            mesh = next(part for part in victim.get_driver_character().get_components_by_class(unreal.SkeletalMeshComponent) if part.get_name() == 'CharacterMesh0')
            AimAt(host, mesh.get_socket_location('head'))
            Command(host, 'RequestFireStart')
            Command(host, 'RequestFireStop', 0.25)
            Phase('friendly_damage')
        elif state['phase'] == 'friendly_damage' and elapsed > 0.8:
            Snapshot()
            TraceDiagnostics()
            Check('Server traces damage an allied occupant', 0 < Combat(state['victim']).get_health() < 100)
            state['car_health'] = state['car'].get_health()
            AimAt(host, state['car'].get_actor_location() + unreal.Vector(0, 0, 65))
            Command(host, 'RequestFireStart')
            Command(host, 'RequestFireStop', 0.25)
            Phase('own_car_damage')
        elif state['phase'] == 'own_car_damage' and elapsed > 0.8:
            Check('Own car blocks weapon trace and takes damage', state['car'].get_health() < state['car_health'])
            Command(first, 'RequestVehicleExit')
            Phase('exit')
        elif state['phase'] == 'exit' and elapsed > 1.0:
            server_first = next(pc for pc in state['server_pcs'] if helper.GetPlayerId(pc) == helper.GetPlayerId(first))
            if server_first.get_seated_vehicle() is not None:
                return
            Check('H exits through existing seating system', first.get_seated_vehicle() is None)
            Check('Walking cannot fire', not server_first.get_driver_character().get_combat_component().can_fire())
            Check('Exit keeps personal reserve', Weapon(server_first, True).get_reserve_ammo() == 120 - state['spent'])
            Snapshot()
            Command(host, 'RequestWindowLeft')
            Phase('prepare_late_join')
        elif state['phase'] == 'prepare_late_join' and elapsed > 1.0:
            state['old_second'] = second
            state['old_second_id'] = helper.GetPlayerId(second)
            unreal.SystemLibrary.execute_console_command(second.get_outer().get_outer(), 'open 127.0.0.1:' + str(state['port']), second)
            Phase('late_join')
        elif state['phase'] == 'late_join':
            local_clients = [unreal.GameplayStatics.get_player_controller(world, 0) for world in clients]
            new_clients = [pc for pc in local_clients if isinstance(pc, unreal.GPlayerController) and helper.GetPlayerId(pc) not in (helper.GetPlayerId(first), state['old_second_id'])]
            if not new_clients or not new_clients[0].get_seated_vehicle():
                return
            late_car = new_clients[0].get_seated_vehicle()
            late_host = late_car.get_seating_component().get_seat_state(0).get_editor_property('occupant')
            if not late_host:
                return
            late_combat = late_host.get_combat_component()
            late_weapon = late_combat.get_weapon(unreal.GWeaponSlot.SECONDARY)
            if not late_weapon or not late_combat.is_outside():
                return
            Check('Late join receives existing outside pose', late_combat.get_window_alpha() > 0.99)
            Check('Late join receives current ammunition', late_weapon.get_bullet_count() == Weapon(host).get_bullet_count())
            Check('Disconnect does not release driver possession', host.get_controlled_pawn() == state['car'])
            Finish()
    except Exception:
        Finish(traceback.format_exc())

def Start():
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    unreal.SystemLibrary.execute_console_command(world, 'log LogVehicleCombat Verbose')
    helper.Tick = Tick
    helper.Finish = Finish
    result = helper.Start()
    result.update({'report': str(report), 'limit_seconds': 600})
    return result

if __name__ == '__main__':
    unreal.EditorPythonScripting.set_keep_python_script_alive(True)
    print(Start())
