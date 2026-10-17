"""Create owner-only hands from Manny; preserve the world mesh and skeleton."""
import json
from pathlib import Path
import unreal


def PrepareFirstPersonMesh():
    source_path = '/Game/M8Shooter/Manny/SKM_Manny_Combat'
    target_path = source_path + '_FirstPerson'
    library = unreal.EditorAssetLibrary
    source = unreal.load_asset(source_path)
    mesh = None
    if library.does_asset_exist(target_path):
        mesh = unreal.load_asset(target_path)
        version = library.get_metadata_tag(mesh, 'M8FirstPersonManny')
        if version not in ('ForearmsV1', 'HandsV2'):
            raise RuntimeError('Unexpected existing first-person mesh: ' + target_path)
        # Keep animated bone bounds even though the component has no collision.
        if mesh.get_editor_property('physics_asset') != source.get_editor_property('physics_asset'):
            mesh.set_editor_property('physics_asset', source.get_editor_property('physics_asset'))
            library.save_loaded_asset(mesh, False)
        if version == 'HandsV2':
            return mesh
    dynamic = unreal.new_object(unreal.DynamicMesh)
    _, outcome = unreal.GeometryScript_AssetUtils.copy_mesh_from_skeletal_mesh(
        source, dynamic, unreal.GeometryScriptCopyMeshFromAssetOptions(),
        unreal.GeometryScriptMeshReadLOD(lod_type=unreal.GeometryScriptLODType.SOURCE_MODEL, lod_index=0))
    if outcome != unreal.GeometryScriptOutcomePins.SUCCESS:
        raise RuntimeError('Cannot read Manny source geometry')
    _, bones = unreal.GeometryScript_BoneWeights.get_all_bones_info(dynamic)
    arm_bones = {bone.index for bone in bones if str(bone.name).startswith(
        ('hand', 'index', 'middle', 'ring', 'pinky', 'thumb'))}
    _, vertices, _ = unreal.GeometryScript_MeshQueries.get_all_vertex_i_ds(dynamic)
    kept_vertices = set()
    for vertex in unreal.GeometryScript_List.convert_index_list_to_array(vertices):
        _, weights, valid = unreal.GeometryScript_BoneWeights.get_vertex_bone_weights(dynamic, vertex)
        if valid and sum(weight.weight for weight in weights if weight.bone_index in arm_bones) >= 0.5:
            kept_vertices.add(vertex)
    _, triangles, _ = unreal.GeometryScript_MeshQueries.get_all_triangle_i_ds(dynamic)
    triangle_ids = list(unreal.GeometryScript_List.convert_index_list_to_array(triangles))
    removed = []
    for triangle in triangle_ids:
        indices, valid = unreal.GeometryScript_MeshQueries.get_triangle_indices(dynamic, triangle)
        if not valid or not all(vertex in kept_vertices for vertex in (indices.x, indices.y, indices.z)):
            removed.append(triangle)
    retained = len(triangle_ids) - len(removed)
    if retained < 100 or retained >= len(triangle_ids) // 2:
        raise RuntimeError('Unexpected hand selection: ' + str(retained))
    _, selection = unreal.GeometryScript_MeshSelection.convert_index_array_to_mesh_selection(
        dynamic, removed, unreal.GeometryScriptMeshSelectionType.TRIANGLES)
    _, deleted = unreal.GeometryScript_MeshEdits.delete_selected_triangles_from_mesh(dynamic, selection)
    if deleted != len(removed):
        raise RuntimeError('Incomplete first-person geometry selection')
    mesh = mesh or library.duplicate_asset(source_path, target_path)
    subsystem = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)
    lod_count = subsystem.get_lod_count(mesh)
    if lod_count > 1 and not subsystem.remove_lods(mesh, list(range(1, lod_count))):
        raise RuntimeError('Cannot remove unused owner-view LODs')
    _, outcome = unreal.GeometryScript_AssetUtils.copy_mesh_to_skeletal_mesh(
        dynamic, mesh, unreal.GeometryScriptCopyMeshToAssetOptions(
            bone_hierarchy_mismatch_handling=unreal.GeometryScriptBoneHierarchyMismatchHandling.REMAP_GEOMETRY_TO_REFERENCE_SKELETON),
        unreal.GeometryScriptMeshWriteLOD(lod_index=0))
    if outcome != unreal.GeometryScriptOutcomePins.SUCCESS or mesh.get_editor_property('skeleton') != source.get_editor_property('skeleton'):
        raise RuntimeError('Cannot preserve Manny first-person skinning')
    library.set_metadata_tag(mesh, 'M8FirstPersonManny', 'HandsV2')
    if not library.save_loaded_asset(mesh, False):
        raise RuntimeError('Cannot save first-person mesh')
    folder = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())) / 'M8Integration20261016'
    folder.mkdir(parents=True, exist_ok=True)
    (folder / 'first_person_mesh.json').write_text(json.dumps({
        'source': source_path, 'target': target_path, 'source_triangles': len(triangle_ids),
        'retained_triangles': retained, 'removed_triangles': deleted,
        'skeleton': mesh.get_editor_property('skeleton').get_path_name(),
        'original_world_mesh_modified': False,
    }, indent=2))
    return mesh


if __name__ == '__main__':
    print(PrepareFirstPersonMesh().get_path_name())
