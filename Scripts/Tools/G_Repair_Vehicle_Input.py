"""Repair the two vehicle key defaults without changing other controller settings."""
import unreal


def RepairVehicleInput():
    if unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor():
        raise RuntimeError('Stop PIE before repairing controller defaults')
    blueprint = unreal.load_asset('/Game/Blueprints/BP_GPlayerController')
    controller = unreal.get_default_object(blueprint.generated_class())
    result = {}
    for field, name in (('seatInteractionKey', 'F'), ('vehicleExitKey', 'H')):
        previous = controller.get_editor_property(field).export_text()
        key = unreal.Key()
        # FKey has its own text importer; it expects the bare key name.
        key.import_text(name)
        if not unreal.InputLibrary.key_is_valid(key) or key.export_text() != name:
            raise RuntimeError('Invalid vehicle input key: ' + name)
        controller.set_editor_property(field, key)
        result[field] = {'previous': previous, 'current': key.export_text()}
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    if blueprint.get_editor_property('status') == unreal.BlueprintStatus.BS_ERROR:
        raise RuntimeError('Controller compilation failed')
    controller = unreal.get_default_object(blueprint.generated_class())
    for field, name in (('seatInteractionKey', 'F'), ('vehicleExitKey', 'H')):
        key = controller.get_editor_property(field)
        if not unreal.InputLibrary.key_is_valid(key) or key.export_text() != name:
            raise RuntimeError('Controller lost key after compilation: ' + name)
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False):
        raise RuntimeError('Controller save failed')
    return result


if __name__ == '__main__':
    print(RepairVehicleInput())
