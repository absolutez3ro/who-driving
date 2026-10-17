"""Real owned-client RPCs, one listen server and two clients; no Editor level writes."""
import json
from pathlib import Path
import time
import traceback
import unreal
import re
reportRoot=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()))/'VehicleIntegration'/'SeatSmokeTests'
reportRoot.mkdir(parents=True,exist_ok=True)
vehicleBlueprint='/Game/Blueprints/Vehicle/BP_GPlayerCar'
testState={}

playModes={'PIE_Standalone':0,'PIE_ListenServer':1,'PIE_Client':2}
settingNames=('PlayNumberOfClients','RunUnderOneProcess','bLaunchSeparateServer')
def Require(condition,message):
	if not condition:
		raise RuntimeError(message)

def ReadPlayMode(settings):
	import uuid
	world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
	token = 'CODEX_PLAY_SETTINGS_' + uuid.uuid4().hex
	unreal.log(token)
	path = settings.get_path_name()
	unreal.SystemLibrary.execute_console_command(world, 'obj dump ' + path)
	unreal.SystemLibrary.execute_console_command(world, 'flushlog')
	folder = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_log_dir()))
	marker = "*** Property dump for object 'LevelEditorPlaySettings " + path + "' ***"
	text = None
	for log in sorted(folder.glob('*.log'),key=lambda p:p.stat().st_mtime,reverse=True):
		candidate=log.read_text(encoding='utf-8',errors='replace')
		if token in candidate and marker in candidate.split(token,1)[1]:
			text=candidate.split(token,1)[1]
			break
	Require(text is not None, 'Fresh current play settings dump missing')
	block=text.rsplit(marker,1)[-1].split('*** Property dump for object',1)[0]
	match=re.search(r'PlayNetMode=(PIE_\w+)',block)
	Require(match and match.group(1) in playModes,'Unknown current network mode')
	return playModes[match.group(1)]

def SetPlayMode(settings, value):
	library = unreal.get_default_object(unreal.SystemLibrary)
	library.call_method('SetBytePropertyByName', (settings, 'PlayNetMode', value))
	Require(ReadPlayMode(settings) == value, 'Network-mode reflection call did not apply')

def ReadProperty(obj,name):
	return obj.get_editor_property(name) if obj else None
def GetPlayerId(controller):
	value=ReadProperty(controller,'player_state')
	return int(ReadProperty(value,'player_id')) if value else None
def GetAvatar(controller):
	return controller.get_driver_character()

def GetVehicle(controller):
	return controller.get_seated_vehicle()
def GetSeatState(car,index):
	value=car.get_seating_component().get_seat_state(index)
	return {'occupant':int(ReadProperty(ReadProperty(value,'playerState'),'player_id')) if ReadProperty(value,'playerState') else None,
	 'reserved':int(ReadProperty(ReadProperty(value,'reservedBy'),'player_id')) if ReadProperty(value,'reservedBy') else None,
	 'character':ReadProperty(value,'occupant')}
def SnapshotSeats(car):
	return [{k:v for k,v in GetSeatState(car,i).items() if k!='character'} for i in range(4)]
def AreReplicasSynchronized():
	expected=SnapshotSeats(testState['car'])
	for car in testState['replicas'].values():
		if SnapshotSeats(car)!=expected:
			return False
		for index in range(4):
			character=GetSeatState(car,index)['character']
			if expected[index]['occupant'] is not None:
				if character is None or character.get_attach_parent_actor()!=car:
					return False
				seat=car.get_seating_component().get_seat_component(index)
				expectedPosition=seat.get_world_transform().transform_location(ReadProperty(seat,'characterOffset').translation)
				if (character.get_actor_location()-expectedPosition).length()>5:
					return False
	return True

def Check(message,condition=True):
	if not condition:
		raise RuntimeError(message)
	if message not in testState['checks']:
		testState['checks'].append(message)
def SetPhase(name):
	testState['phase']=name
	testState['phase_start']=time.monotonic()
def ScheduleAction(controller,function):
	info=unreal.LatentActionInfo()
	value='(Linkage=0,UUID='+str(int(time.monotonic()*10000)%2000000000)+',ExecutionFunction=\"'+function+'\",CallbackTarget=\"'+controller.get_path_name()+'\")'
	info.import_text(value)
	unreal.get_default_object(unreal.SystemLibrary).call_method('Delay',args=(controller,0.15,info))
def RequestSeatAtCamera(controller,index):
	car=testState['car'] if controller.has_authority() else testState['replicas'][GetPlayerId(controller)]
	character=GetAvatar(controller)
	Check('Requesting controller has an avatar',character is not None)
	camera=character.get_component_by_class(unreal.CameraComponent)
	origin=camera.get_world_location()
	target=car.get_seating_component().get_seat_component(index).get_world_location()+unreal.Vector(0,0,20)
	desired=(target-origin).rotator()
	current=camera.get_world_rotation()
	deltaYaw=(desired.yaw-current.yaw+180)%360-180
	sensitivity=character.get_editor_property('CameraSensitivity')
	character.do_aim(deltaYaw/sensitivity,(desired.pitch-current.pitch)/sensitivity)
	if GetVehicle(controller) is None:
		controller.set_control_rotation(unreal.Rotator(pitch=0,yaw=desired.yaw,roll=0))
	testState.setdefault('pending_look',[]).append((controller,index))
	if 'views' not in testState: testState['views']=[]
	testState['views'].append({'player':GetPlayerId(controller),'camera':str(origin),'player_view':str(controller.get_player_view_point()),'target':str(target),'seat':index})

def PositionOutside(serverController,index):
	character=GetAvatar(serverController)
	x=22 if index<2 else -110
	y=-230 if index%2==0 else 230
	point=testState['car'].get_actor_transform().transform_location(unreal.Vector(x,y,110))
	character.set_actor_location(point,False,True)
def Start():
	checkLevel=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
	if checkLevel.is_in_play_in_editor() or testState:
		raise RuntimeError('Preserve existing PIE')
	settings=unreal.load_object(None,'/Script/UnrealEd.Default__LevelEditorPlaySettings')
	world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
	mode=unreal.get_default_object(world.get_world_settings().get_editor_property('default_game_mode'))
	testState.update({'started':time.monotonic(),'settings':settings,'old_mode':ReadPlayMode(settings),
	 'old_settings':{n:ReadProperty(settings,n) for n in settingNames},'mode':mode,
	 'old_pawn':ReadProperty(mode,'default_pawn_class'),'checks':[],'phase':'initial','phase_start':time.monotonic(),
	 'samples':[],'replicas':{},'class':unreal.load_asset(vehicleBlueprint).generated_class(),
	 'port':int(ReadProperty(settings,'ServerPort'))})
	(reportRoot/'new_car_original_play_settings.json').write_text(json.dumps({'mode':testState['old_mode'],'settings':testState['old_settings'],'pawn':testState['old_pawn'].get_path_name()},indent=2),encoding='utf-8')
	try:
		controllerBlueprint=unreal.load_asset('/Game/Blueprints/BP_GPlayerController')
		controllerDefaults=unreal.get_default_object(controllerBlueprint.generated_class())
		for field,name in (('seatInteractionKey','F'),('vehicleExitKey','H')):
			key=controllerDefaults.get_editor_property(field)
			Check('Controller has valid '+name+' binding for '+field,
				unreal.InputLibrary.key_is_valid(key) and key.export_text()==name)
		SetPlayMode(settings,1)
		settings.set_editor_property('PlayNumberOfClients',3)
		settings.set_editor_property('RunUnderOneProcess',True)
		settings.set_editor_property('bLaunchSeparateServer',False)
		mode.set_editor_property('default_pawn_class',testState['class'])
		testState['callback']=unreal.register_slate_post_tick_callback(Tick)
		checkLevel.editor_request_begin_play()
	except Exception:
		Finish(traceback.format_exc())
		raise
	return {'status':'requested','report':str(reportRoot/'new_car_network_result.json'),'clients':2,'limit_seconds':120}
def Finish(error=None):
	report={'error':error,'checks':testState.get('checks'),'phase':testState.get('phase'),'samples':testState.get('samples'),
	 'original_settings':testState.get('old_settings'),'original_mode':testState.get('old_mode'),'initial_snapshot':testState.get('initial_snapshot'),'client_info':testState.get('client_info'),'views':testState.get('views'),'pending_debug':testState.get('pending_debug'),'view_after':testState.get('view_after'),'exit_server_retry':testState.get('exit_server_retry'), 'passed':error is None and testState.get('phase')=='rejoined','scope':'listen server and two owned clients; vehicle physics parked during seat tests'}
	try:
		if testState.get('car'):
			report['car_transform']=str(testState['car'].get_actor_transform())
			report['avatars']='Detailed live avatar data omitted after reconnect'
		testState['mode'].set_editor_property('default_pawn_class',testState['old_pawn'])
		SetPlayMode(testState['settings'],testState['old_mode'])
		for n,v in testState['old_settings'].items():
			testState['settings'].set_editor_property(n,v)
		report['settings_restored']=True
	except Exception:
		report['restoration_error']=traceback.format_exc()
	if testState.get('callback') is not None:
		unreal.unregister_slate_post_tick_callback(testState['callback'])
	unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play()
	(reportRoot/'new_car_network_result.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
	testState.clear()
def Tick(delta):
	try:
		now=time.monotonic()
		if now-testState['started']>120 or now-testState['phase_start']>18:
			raise RuntimeError('Timed out: '+testState['phase'])
		if testState.get('car'):
			for parked in [testState['car']]+list(testState['replicas'].values()):
				try:
					if not unreal.SystemLibrary.is_valid(parked):
						continue
				except Exception:
					continue
				parked.get_movement_component().set_component_tick_enabled(False)
				root=next(c for c in parked.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name()=='VehicleMesh')
				root.set_all_bodies_simulate_physics(False)
				parked.set_actor_location(unreal.Vector(-900,-800,20),False,True)
				parked.set_actor_rotation(unreal.Rotator(),True)
		remaining=[]
		for controller,index in testState.get('pending_look',[]):
			character=GetAvatar(controller)
			car=testState['car'] if controller.has_authority() else testState['replicas'][GetPlayerId(controller)]
			camera=character.get_component_by_class(unreal.CameraComponent)
			origin=camera.get_world_location()
			target=car.get_seating_component().get_seat_component(index).get_world_location()+unreal.Vector(0,0,20)
			desired=(target-origin).rotator()
			current=camera.get_world_rotation()
			sensitivity=character.get_editor_property('CameraSensitivity')
			character.do_aim(((desired.yaw-current.yaw+180)%360-180)/sensitivity,(desired.pitch-current.pitch)/sensitivity)
			if GetVehicle(controller) is None:
				controller.set_control_rotation(unreal.Rotator(pitch=0,yaw=desired.yaw,roll=0))
			actualOrigin,actualRotation=controller.get_player_view_point()
			testState.setdefault('pending_debug',{})[GetPlayerId(controller)]={'actor':str(character.get_actor_rotation()),'parent':str(character.get_attach_parent_actor()),'root_camera':str(camera.get_attach_parent().get_editor_property('relative_rotation')),'camera':str(camera.get_world_rotation()),'view':str(actualRotation),'desired':str(desired),'origin_error':(actualOrigin-origin).length(),'control':str(controller.get_control_rotation()),'location':str(character.get_actor_location())}
			yawError=abs((actualRotation.yaw-desired.yaw+180)%360-180)
			if yawError<1 and abs(actualRotation.pitch-desired.pitch)<1 and (actualOrigin-origin).length()<5:
				ScheduleAction(controller,'RequestLookedAtSeat')
			else:
				remaining.append((controller,index))
		testState['pending_look']=remaining
		worlds=unreal.EditorLevelLibrary.get_pie_worlds(True)
		servers=[w for w in worlds if unreal.GameplayStatics.get_game_mode(w)]
		clients=[w for w in worlds if not unreal.GameplayStatics.get_game_mode(w)]
		if not servers or len(clients)<2:
			return
		server=servers[0]
		pcs=unreal.GameplayStatics.get_all_actors_of_class(server,unreal.GPlayerController)
		localClients=[unreal.GameplayStatics.get_player_controller(w,0) for w in clients]
		if any(pc is None or GetPlayerId(pc) is None for pc in localClients):
			return
		if testState['phase']=='initial':
			initial=[{'pid':GetPlayerId(pc),'pc':pc.get_path_name(),'pawn':pc.get_controlled_pawn().get_path_name() if pc.get_controlled_pawn() else None,'vehicle':GetVehicle(pc).get_path_name() if GetVehicle(pc) else None,'avatar':GetAvatar(pc).get_path_name() if GetAvatar(pc) else None, 'seats':SnapshotSeats(pc.get_controlled_pawn()) if isinstance(pc.get_controlled_pawn(),unreal.GPlayerCar) else None} for pc in pcs]
			testState['initial_snapshot']=initial
			if len(pcs)!=3 or any(GetAvatar(pc) is None or GetVehicle(pc) is None for pc in pcs):
				return
			if any(GetAvatar(pc) is None or GetVehicle(pc) is None for pc in localClients):
				return
			host=unreal.GameplayStatics.get_player_controller(server,0)
			car=GetVehicle(host)
			testState.update({'server':server,'host':host,'car':car,'client_pcs':{GetPlayerId(pc):pc for pc in localClients},
			 'server_pcs':{GetPlayerId(pc):pc for pc in pcs},'host_id':GetPlayerId(host)})
			testState['remote_ids']=sorted(testState['client_pcs'])
			for pc in localClients:
				replica=next((a for a in unreal.GameplayStatics.get_all_actors_of_class(pc.get_outer().get_outer(),testState['class']) if ReadProperty(a,'player_state') and ReadProperty(ReadProperty(a,'player_state'),'player_id')==GetPlayerId(host)),None)
				if not replica:
					return
				testState['replicas'][GetPlayerId(pc)]=replica
			allCharacters=unreal.GameplayStatics.get_all_actors_of_class(server,unreal.Whos_DrivingCharacter)
			Check('Exactly one server-spawned avatar per player',len(allCharacters)==3)
			parts=car.get_components_by_class(unreal.SkeletalMeshComponent)
			mesh=next(c for c in parts if c.get_name()=='VehicleMesh')
			Check('All 29 vehicle parts use the permanent C++ leader pose',len(parts)==29 and all(c==mesh or ReadProperty(c,'leader_pose_component')==mesh for c in parts))
			for i,pc in enumerate(pcs):
				own=GetVehicle(pc)
				own.get_movement_component().set_component_tick_enabled(False)
				root=next(c for c in own.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name()=='VehicleMesh')
				root.set_simulate_physics(False)
				root.set_all_bodies_simulate_physics(False)
				location=car.get_actor_location() if own==car else car.get_actor_location()+unreal.Vector(0,750*(i+1),0)
				own.set_actor_location(unreal.Vector(-900,-800,20),False,True)
				own.set_actor_rotation(unreal.Rotator(),True)
			testState['client_info']=[{'pid':GetPlayerId(pc),'authority':pc.has_authority(),'local':pc.is_local_controller(),'pawn':pc.get_controlled_pawn().get_path_name() if pc.get_controlled_pawn() else None} for pc in localClients]
			for pc in localClients:
				ScheduleAction(pc,'RequestVehicleExit')
			SetPhase('remote_exit')
			return
		current=testState['phase']
		car=testState['car']
		compactState=SnapshotSeats(car)
		if not testState['samples'] or testState['samples'][-1]['state']!=compactState:
			testState['samples'].append({'phase':current,'state':compactState})
		ids=testState['remote_ids']
		first=testState['client_pcs'][ids[0]]
		second=testState['client_pcs'][ids[1]]
		if current=='remote_exit':
			if all(GetVehicle(testState['server_pcs'][i]) is None and GetVehicle(testState['client_pcs'][i]) is None for i in ids):
				RequestSeatAtCamera(testState['host'],1)
				SetPhase('host_reserved')
		elif current=='host_reserved':
			if now-testState['phase_start']>0.5:
				testState['view_after']={'view':str(testState['host'].get_player_view_point()),'camera':str(GetAvatar(testState['host']).get_component_by_class(unreal.CameraComponent).get_world_rotation())}
				box=car.get_seating_component().get_seat_component(1)
				testState['view_after']['box']={'enabled':str(box.get_collision_enabled()),'response':str(box.get_collision_response_to_channel(unreal.CollisionChannel.ECC_VEHICLE_SEAT_INTERACTION)),'location':str(box.get_world_location()),'extent':str(box.get_scaled_box_extent())}
				try:
					origin,rotation=testState['host'].get_player_view_point()
					hit=unreal.SystemLibrary.line_trace_single(testState['host'],origin,origin+unreal.Vector(__import__('math').cos(__import__('math').radians(rotation.pitch))*__import__('math').cos(__import__('math').radians(rotation.yaw)),__import__('math').cos(__import__('math').radians(rotation.pitch))*__import__('math').sin(__import__('math').radians(rotation.yaw)),__import__('math').sin(__import__('math').radians(rotation.pitch)))*500,unreal.TraceTypeQuery.ECC_VEHICLE_SEAT_INTERACTION,False,[GetAvatar(testState['host'])],unreal.DrawDebugTrace.NONE,True)
					testState['view_after']['trace']=hit.export_text() if hit else None
				except Exception as error:
					testState['view_after']['trace_error']=str(error)
			seatSnapshot=GetSeatState(car,1)
			if seatSnapshot['reserved']==testState['host_id']:
				Check('Destination reserved while original driver seat remains occupied',GetSeatState(car,0)['occupant']==testState['host_id'] and seatSnapshot['occupant'] is None)
				if not testState.get('cancel_checked'):
					testState['cancel_checked']=True
					ScheduleAction(testState['host'],'RequestVehicleExit')
					SetPhase('host_cancelled')
				else:
					SetPhase('host_passenger')
		elif current=='host_cancelled':
			if GetSeatState(car,1)['reserved'] is None and AreReplicasSynchronized():
				Check('H cancels a pending transfer without releasing the original seat',GetSeatState(car,0)['occupant']==testState['host_id'] and GetVehicle(testState['host'])==car)
				movement=car.get_movement_component()
				movement.set_throttle_input(0.7)
				movement.call_method('ServerUpdateState',args=(0.2,0.7,0.0,0.0,1,0.0,0.0,0.0))
				Check('Regression precondition: throttle applied and a previous network input packet injected',movement.get_throttle_input()>0.6)
				RequestSeatAtCamera(testState['host'],1)
				SetPhase('host_reserved')
		elif current=='host_passenger':
			if GetSeatState(car,1)['occupant']==testState['host_id'] and all(GetVehicle(testState['server_pcs'][i]) is None for i in ids) and AreReplicasSynchronized():
				Check('Driver to passenger transfer replicates to both clients',testState['host'].get_controlled_pawn()==GetAvatar(testState['host']) and GetSeatState(car,0)['occupant'] is None)
				movement=car.get_movement_component()
				Check('Leaving the driver seat clears throttle and engages the handbrake',abs(movement.get_throttle_input())<0.001 and movement.get_handbrake_input())
				PositionOutside(testState['server_pcs'][ids[0]],0)
				PositionOutside(testState['server_pcs'][ids[1]],2)
				SetPhase('approach_driver')
		elif current=='approach_driver' and now-testState['phase_start']>0.7:
			RequestSeatAtCamera(first,0)
			SetPhase('client_driver')
		elif current=='client_driver':
			if GetSeatState(car,0)['occupant']==ids[0] and GetVehicle(first)==testState['replicas'][ids[0]] and AreReplicasSynchronized():
				Check('Owned client RPC boards driver seat and transfers vehicle possession',testState['server_pcs'][ids[0]].get_controlled_pawn()==car and first.get_controlled_pawn()==testState['replicas'][ids[0]])
				PositionOutside(testState['server_pcs'][ids[1]],2)
				SetPhase('prepare_race')
		elif current=='prepare_race' and now-testState['phase_start']>0.7:
			RequestSeatAtCamera(first,2)
			RequestSeatAtCamera(second,2)
			SetPhase('race_reserved')
		elif current=='race_reserved':
			reserved=GetSeatState(car,2)['reserved']
			if reserved in ids:
				testState['winner']=reserved
				testState['loser']=next(i for i in ids if i!=reserved)
				Check('Two clients contest one seat; server creates one reservation',GetSeatState(car,2)['occupant'] is None)
				SetPhase('race_completed')
		elif current=='race_completed':
			if GetSeatState(car,2)['occupant']==testState['winner'] and AreReplicasSynchronized():
				loserSeat=0 if testState['loser']==ids[0] else None
				actual=car.get_seating_component().get_character_seat(GetAvatar(testState['server_pcs'][testState['loser']]))
				Check('Exactly one contest winner; losing player keeps original seat',actual==(loserSeat if loserSeat is not None else -1))
				RequestSeatAtCamera(testState['client_pcs'][testState['loser']],1)
				SetPhase('occupied_rejected')
		elif current=='occupied_rejected' and now-testState['phase_start']>1.3:
			Check('Occupied front passenger seat cannot be stolen',GetSeatState(car,1)['occupant']==testState['host_id'] and GetSeatState(car,1)['reserved'] is None)
			RequestSeatAtCamera(testState['client_pcs'][testState['winner']],3)
			SetPhase('rear_right')
		elif current=='rear_right':
			if GetSeatState(car,3)['occupant']==testState['winner'] and AreReplicasSynchronized():
				Check('Both rear window seats work without splitting the bench',GetSeatState(car,2)['occupant'] is None and car.get_seating_component().get_seat_component(4) is None)
				if GetSeatState(car,0)['occupant'] is not None:
					ScheduleAction(first,'RequestVehicleExit')
				SetPhase('free_driver')
		elif current=='free_driver':
			if GetSeatState(car,0)['occupant'] is None:
				RequestSeatAtCamera(testState['client_pcs'][testState['winner']],0)
				SetPhase('passenger_driver')
		elif current=='passenger_driver':
			if GetSeatState(car,0)['occupant']==testState['winner'] and AreReplicasSynchronized():
				Check('Passenger to driver transfer gives control only to the new driver',testState['server_pcs'][testState['winner']].get_controlled_pawn()==car and GetSeatState(car,3)['occupant'] is None)
				seat=car.get_seating_component().get_seat_component(0)
				testState['exit_location']=ReadProperty(seat,'exitLocation').copy()
				blocks=unreal.GameplayStatics.get_all_actors_of_class(server,unreal.StaticMeshActor)
				Check('Blocking geometry exists for exit test',len(blocks)>0)
				center,_=blocks[0].get_actor_bounds(True)
				seat.set_editor_property('exitLocation',car.get_actor_transform().inverse_transform_location(center))
				ScheduleAction(testState['client_pcs'][testState['winner']],'RequestVehicleExit')
				SetPhase('blocked_exit')
		elif current=='blocked_exit' and now-testState['phase_start']>0.7:
			Check('Blocked exit preserves occupant and driver possession',GetSeatState(car,0)['occupant']==testState['winner'] and testState['server_pcs'][testState['winner']].get_controlled_pawn()==car)
			car.get_seating_component().get_seat_component(0).set_editor_property('exitLocation',testState['exit_location'])
			ScheduleAction(testState['client_pcs'][testState['winner']],'RequestVehicleExit')
			SetPhase('exit')
		elif current=='exit':
			ownCharacter=GetAvatar(testState['client_pcs'][testState['winner']])
			clientReady=ownCharacter and ownCharacter.get_attach_parent_actor() is None and ownCharacter.get_component_by_class(unreal.CapsuleComponent).get_collision_enabled()!=unreal.CollisionEnabled.NO_COLLISION
			if clientReady and GetSeatState(car,0)['occupant'] is None and GetVehicle(testState['client_pcs'][testState['winner']]) is None and AreReplicasSynchronized():
				character=GetAvatar(testState['server_pcs'][testState['winner']])
				Check('Exit restores character possession, collision and movement',testState['server_pcs'][testState['winner']].get_controlled_pawn()==character and character.get_attach_parent_actor() is None and character.get_component_by_class(unreal.CapsuleComponent).get_collision_enabled()!=unreal.CollisionEnabled.NO_COLLISION and 'NONE' not in str(character.get_component_by_class(unreal.CharacterMovementComponent).get_editor_property('movement_mode')))
				PositionOutside(testState['server_pcs'][ids[1]],2)
				SetPhase('prepare_disconnect')
		elif current=='prepare_disconnect' and now-testState['phase_start']>0.7:
			RequestSeatAtCamera(second,2)
			SetPhase('disconnect_occupied')
		elif current=='disconnect_occupied':
			if GetSeatState(car,2)['occupant']==ids[1] and AreReplicasSynchronized():
				testState['old_second']=second
				testState['second_world']=second.get_outer().get_outer()
				unreal.SystemLibrary.execute_console_command(second.get_outer().get_outer(),'open 127.0.0.1:'+str(testState['port']),second)
				SetPhase('rejoin')
		elif current=='rejoin':
			newPcs=[pc for pc in localClients if pc!=testState['old_second'] and GetPlayerId(pc)!=ids[0]]
			if newPcs and GetSeatState(car,2)['occupant'] is None:
				pc=newPcs[0]
				replicas=unreal.GameplayStatics.get_all_actors_of_class(pc.get_outer().get_outer(),testState['class'])
				target=min(replicas,key=lambda a:(a.get_actor_location()-car.get_actor_location()).length()) if replicas else None
				if target and SnapshotSeats(target)==SnapshotSeats(car):
					Check('Disconnect frees occupied seat and reconnect receives current occupancy')
					SetPhase('rejoined')
					Finish()
	except Exception:
		Finish(traceback.format_exc())

if __name__ == "__main__":
	unreal.EditorPythonScripting.set_keep_python_script_alive(True)
	print(Start())
