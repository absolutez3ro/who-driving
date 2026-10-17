# M8 vehicle combat integration

## Scope and source

M8 is a donor, not a replacement GameMode, controller or character framework. The playable character remains Manny. The existing vehicle, four-seat reservations and possession rules remain the integration points.

The source project is `E:/_Projects/UE5 Projects/M8s Stylized Shooter Kit`. Resources were copied through an isolated staging project with package references remapped to `/Game/M8Shooter`. The donor's unrelated AI, survival, inventory and shop frameworks are not dependencies of the integrated weapons.

167 resource packages were transferred. All 78 donor character animation sequences were retargeted onto the existing Manny skeleton through `IK_M8`, `IK_Manny` and `RTG_M8_Manny`. Weapon skeleton animations remain on their weapon skeleton. The original source animations are also retained for comparison. Manny uses a mesh copy with mesh-only weapon/magazine sockets; the shared mannequin skeleton was not edited to add sockets.

The two donor weapon Blueprints contain no gameplay nodes beyond their construction entry. Their settings are represented by `DA_M8_Rifle` and `DA_M8_Pistol`; editable weapon children are `BP_M8_Rifle` and `BP_M8_Pistol`. Donor damage, fire intervals, modes, magazines, reserves, reload/equip timing, sounds, particles and animation references were transferred from the audited defaults. Rifle capacity stays 40; its donor attachment list inconsistently describes a 36-round magazine whose name does not match the selected attachment.

## Runtime integration

`UGVehicleCombatComponent`, a default subobject of the existing character, owns the loadout, selected slot, health, aiming and replicated window transition. It does not transfer weapons between players when seats or vehicle possession change. `AGVehicleWeapon` extends the project's existing `AShooterWeapon` and reuses its first/third-person skeletal meshes. Each weapon owns replicated rounds, reserve, fire mode and reload state.

`AGPlayerController` is the owned RPC entry point while controlling either the avatar or the car. It resolves the player's current avatar on the server. Clients submit input and a normalized aim direction, not a hit actor, hit location, damage value or magazine count. The server computes origins from the avatar/weapon, applies spread, checks camera and muzzle traces, enforces per-weapon fire intervals and spends ammunition. Reliable requests control trigger/window/equip/reload; aim updates are unreliable and sent at most 20 times per second by the local controller.

Allied players and the shooter's car remain in weapon traces and can take damage. Character body/head boxes are query-only and do not re-enable passenger movement collisions. Car wheel hit boxes use existing wheel bones and dimensions, route point damage into `DamageWheel`, and do not affect suspension physics. The current chassis collision remains the simplified physics hull already used by the vehicle; it is not a detailed glass/door damage simulation.

`UGVehicleMovementComponent` stores the throttle when the driver starts leaning. While locked, it enforces zero steering/brake/handbrake and the stored throttle before and after Chaos input processing. Possession stays on the car. Returning inside releases the lock once the transition completes. Actual exit/disconnect still follows the existing stop-and-release path.

`ABP_Manny_Vehicle_Combat_IK` uses editable sequence evaluation, upper-body layering, bone transforms and two-bone hand IK. C++ exposes presentation values; the AnimGraph evaluates and blends poses. Retargeted hand reload/equip poses, donor weapon animations, magazine notifies, procedural recoil curves and camera shake provide presentation. Seated leg/pelvis offsets, hand targets and per-seat window/camera offsets are tunable. The window motion is a procedural prototype, not a newly authored cinematic animation. Walking restores the original world and first-person animation classes. The earlier `ABP_Manny_Vehicle_Combat` graph is retained for comparison.

Trigger requests use the current first-person camera direction, matching continuous aim updates. The previous camera-manager cache can lag a frame during input and must not supply the initial shot direction. First-person pose offsets affect the owner's visual mesh; authoritative muzzle traces use the world weapon mesh.

`ABP_Manny_Vehicle_Combat_IK_FirstPerson` is the separate owner view graph. It permits visual arm extension to reach camera-space hand targets while the world graph preserves Manny's limb lengths. The torso is not translated towards the lens. Hand rotation compensates the donor gun's +Y forward axis and the Manny grip socket. Gameplay hit tests continue to use the world weapon. Camera and seated pelvis height were calibrated against the imported doors rather than the old low pose.

The owner view uses `SKM_Manny_Combat_FirstPerson`, a derived copy retaining Manny's hands, skin weights, materials and existing skeleton. The full world mesh is unchanged. `Scripts/Tools/G_Prepare_Manny_First_Person.py` creates this copy by bone weights, removes unused owner-view LODs and refuses to overwrite an unrelated asset. This avoids torso/shoulder geometry crossing the camera while leaning. The preparation report records 23,026 retained triangles from 92,178 source triangles. Animated bounds are retained and expanded on the non-colliding owner component so the hands are not culled behind the door.

## Controls

| Key | Action |
| --- | --- |
| 1 / 2 | Primary / secondary weapon |
| LMB | Fire |
| RMB | Aim |
| R | Reload |
| V | Cycle available fire modes |
| Q | Left seats: outside; right seats: inside |
| E | Right seats: outside; left seats: inside |
| F | Interact / request the looked-at seat |
| H | Exit; cancel a pending seat reservation first |

Passengers fire only fully outside. On foot, firing is disabled. The driver can fire secondary inside and use primary outside; returning inside selects secondary only for the driver. Seat changes and exits cancel trigger, aim, reload and window transitions.

## Assets and backups

- `/Game/M8Shooter/Weapons/DA_M8_Rifle`
- `/Game/M8Shooter/Weapons/DA_M8_Pistol`
- `/Game/M8Shooter/Weapons/BP_M8_Rifle`
- `/Game/M8Shooter/Weapons/BP_M8_Pistol`
- `/Game/M8Shooter/Manny/SKM_Manny_Combat`
- `/Game/M8Shooter/Manny/SKM_Manny_Combat_FirstPerson`
- `/Game/M8Shooter/Manny/ABP_Manny_Vehicle_Combat_IK`
- `/Game/M8Shooter/Manny/ABP_Manny_Vehicle_Combat_IK_FirstPerson`
- `/Game/M8Shooter/Manny/Animations`
- `/Game/M8Shooter/Backups/BP_FirstPersonCharacter_PreCombat`
- `/Game/M8Shooter/Backups/BP_GPlayerController_PreCombat`

The conflicting pre-existing projectile object channel was moved to `ECC_GameTraceChannel2`. Seat interaction remains on channel 1; weapon traces use channel 3. The inactive template shooter continues to use the `Projectile` collision profile.

Migration, retarget, configuration and test artifacts are under `Whos_Driving/Saved/M8Integration20261016`. `resource_mapping.json` and `retarget_mapping.json` preserve source-to-destination provenance. `donor_missing_optional_references.json` records four already-missing donor references (old preview meshes/UI assets); they were not fabricated or copied from another project.

## Verification

Runtime and Editor development builds passed on 2026-10-16. The real listen-server/two-client combat smoke test passed 29 checks, including actual allied occupant and own-car weapon traces, independent ammunition, exact reserve subtraction, driver throttle retention/release, possession, exit and late join. The seating smoke test also passed after combat integration, covering reservations, contested seats, seat transfers, blocked exits, disconnect and reconnect. Both tests restore prior play settings and GameMode defaults and park vehicle physics during their checks; they are not a moving-vehicle latency/load test.

Resource validation found 259 integration packages, 78 retargeted character sequences, no broken hard package references, no `/Script/POLYHIT` dependencies and no changes to donor files. Both active animation Blueprints compiled successfully. Four pre-existing optional donor references are recorded separately.

Native automation: `Automation RunTests WhosDriving.` covers combat server rules and the existing wheel damage/visual replacement tests. Editor Python entry points: `Scripts/Tests/G_Vehicle_Combat_Smoke_Test.py` and `Scripts/Tests/G_Vehicle_Seating_Smoke_Test.py`. Run them in the open project with PIE stopped. Setting `Saved/M8Integration20261016/capture_visuals.flag` enables slower first-person/observer screenshots during combat checks.

All three native tests passed in the final development build. The combat fixture reports one warning when tearing down its isolated world after BeginPlay; no assertions failed. The seating regression includes two additional checks for valid serialized F/H keys, alongside its 18 behavioral checks. The combat run passed all 29 checks. First-person hip/aim/reload and observer screenshots are saved beside the combat report. The GameMode controller reference was explicitly compiled and saved after the controller Blueprint update; a subsequent cold Editor launch successfully created the project controller for the local host.

The F/H regression was caused by passing `(KeyName="F")` to `unreal.Key.import_text`. FKey's custom text importer expects the bare key name; the struct-looking string becomes an invalid `(` key. `Scripts/Tools/G_Repair_Vehicle_Input.py` repairs and saves only the controller's two key defaults and verifies them after compilation. The integration configuration now uses bare `F`/`H` and validates both keys before saving. The seating smoke test checks these defaults before beginning PIE; earlier behavioral tests invoked controller requests directly and therefore did not catch malformed physical key bindings.

Latest machine-readable reports are `Saved/M8Integration20261016/network_result.json`, `Saved/VehicleIntegration/SeatSmokeTests/new_car_network_result.json` and `Saved/M8Integration20261016/resource_validation.json`. The first-person hands and seated/window/reload poses are an adjustable gameplay prototype and still need animation art polish, including the wrist boundaries and magazine grip. The current implementation does not include a weapon-selection HUD, respawn system or a detailed glass/door ballistic model. A moving-vehicle latency/load scenario has not been validated by these parked-vehicle smoke tests.
