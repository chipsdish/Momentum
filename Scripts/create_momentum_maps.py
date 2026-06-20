import unreal


MAP_DIR = "/Game/Maps"
COURSE_TAG = "MomentumCourseStatic"


def course_tag() -> unreal.Name:
    return unreal.Name(COURSE_TAG)


def ensure_dir(path: str) -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def spawn_label(text: str, location: unreal.Vector, size: float = 96.0) -> None:
    actor = spawn_script_actor("/Script/Momentum.ParkourWorldLabel", f"Label - {text.splitlines()[0]}", (location.x, location.y, location.z))
    if actor:
        actor.set_label_text(text, max(size * 0.58, 18.0))


def spawn_cube(name: str, location: unreal.Vector, rotation: unreal.Rotator, scale: unreal.Vector) -> None:
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, location, rotation)
    actor.set_actor_label(name)
    actor.set_actor_scale3d(scale)
    actor.set_editor_property("tags", [course_tag()])

    mesh_component = actor.get_component_by_class(unreal.StaticMeshComponent)
    cube_mesh = unreal.load_asset("/Engine/BasicShapes/Cube")
    if mesh_component and cube_mesh:
        mesh_component.set_static_mesh(cube_mesh)


def spawn_lighting() -> None:
    sun = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.DirectionalLight,
        unreal.Vector(0.0, 0.0, 600.0),
        unreal.Rotator(-45.0, -35.0, 0.0),
    )
    if sun:
        sun.set_actor_label("Sun Light / 基础光照")
        sun.set_editor_property("tags", [course_tag()])
        light_component = sun.get_component_by_class(unreal.DirectionalLightComponent)
        if light_component:
            light_component.set_editor_property("intensity", 6.0)

    sky = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.SkyLight,
        unreal.Vector(0.0, 0.0, 300.0),
        unreal.Rotator(0.0, 0.0, 0.0),
    )
    if sky:
        sky.set_actor_label("Sky Light / 环境光")
        sky.set_editor_property("tags", [course_tag()])
        sky_component = sky.get_component_by_class(unreal.SkyLightComponent)
        if sky_component:
            sky_component.set_editor_property("intensity", 0.75)


def spawn_course_cube(name: str, location, rotation, dimensions) -> None:
    actor = spawn_script_actor("/Script/Momentum.ParkourBuildPiece", name, location, rotation)
    if actor:
        actor.set_dimensions(unreal.Vector(*dimensions))
        actor.set_label_text(name)


def spawn_script_actor(class_path: str, name: str, location, rotation=(0.0, 0.0, 0.0)):
    actor_class = unreal.load_class(None, class_path)
    if not actor_class:
        unreal.log_warning(f"Could not load {class_path}")
        return None

    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(actor_class, unreal.Vector(*location), unreal.Rotator(*rotation))
    if actor:
        actor.set_actor_label(name)
        actor.set_editor_property("tags", [course_tag()])
    return actor


def set_first_box_extent(actor, extent) -> None:
    if not actor:
        return

    box = actor.get_component_by_class(unreal.BoxComponent)
    if box:
        box.set_box_extent(unreal.Vector(*extent), True)


def create_level_if_missing(asset_path: str) -> bool:
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        unreal.log(f"{asset_path} already exists; skipping")
        return False

    unreal.EditorLevelLibrary.new_level(asset_path)
    return True


def create_main_menu() -> None:
    if not create_level_if_missing(f"{MAP_DIR}/MainMenu"):
        return

    spawn_label("Momentum\n主菜单", unreal.Vector(0.0, 0.0, 220.0), 120.0)
    spawn_label("Play 后显示中文主菜单\n测试关卡 / 退出游戏", unreal.Vector(0.0, 0.0, 40.0), 52.0)
    spawn_cube("MainMenu Floor", unreal.Vector(0.0, 0.0, -60.0), unreal.Rotator(0.0, 0.0, 0.0), unreal.Vector(12.0, 12.0, 0.4))
    spawn_lighting()
    unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0.0, -420.0, 120.0), unreal.Rotator(0.0, 90.0, 0.0))
    unreal.EditorLoadingAndSavingUtils.save_current_level()


def create_test_level() -> None:
    if not create_level_if_missing(f"{MAP_DIR}/TestLevel"):
        return

    spawn_label("Momentum TestLevel\n静态灰盒赛道，可在编辑器直接调整", unreal.Vector(0.0, -650.0, 300.0), 72.0)
    spawn_lighting()

    unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0.0, 0.0, 180.0), unreal.Rotator(0.0, 0.0, 0.0))

    spawn_course_cube("Start Platform", (0.0, 0.0, 0.0), (0.0, 0.0, 0.0), (600.0, 600.0, 80.0))
    spawn_label("Start / 起点", unreal.Vector(0.0, -360.0, 160.0), 64.0)

    spawn_course_cube("Walkable Slope", (1300.0, 0.0, 60.0), (18.0, 0.0, 0.0), (900.0, 420.0, 70.0))
    spawn_label("小坡: 正常 Grounded Movement", unreal.Vector(1300.0, -520.0, 260.0), 48.0)

    spawn_course_cube("Bunny Hop Block 1", (2500.0, 0.0, 70.0), (0.0, 0.0, 0.0), (250.0, 250.0, 70.0))
    spawn_course_cube("Bunny Hop Block 2", (3300.0, 0.0, 70.0), (0.0, 0.0, 0.0), (250.0, 250.0, 70.0))
    spawn_course_cube("Bunny Hop Block 3", (4100.0, 0.0, 70.0), (0.0, 0.0, 0.0), (250.0, 250.0, 70.0))
    spawn_label("Bunny Hop / 手动连跳", unreal.Vector(3300.0, -520.0, 240.0), 48.0)

    spawn_course_cube("Surf Ramp Left", (5600.0, -560.0, 170.0), (44.0, -10.0, 0.0), (1200.0, 500.0, 70.0))
    spawn_label("左侧大斜坡: 进入 Surf / Slide", unreal.Vector(5600.0, -1120.0, 430.0), 48.0)

    spawn_course_cube("Acceleration Ramp Right", (7400.0, 560.0, 260.0), (50.0, 10.0, 0.0), (1400.0, 500.0, 70.0))
    spawn_label("右侧真实加速坡: 几何斜坡 + 重力分量\n不是 AddImpulse", unreal.Vector(7400.0, 1120.0, 560.0), 46.0)

    spawn_course_cube("Curved Slide Left 1", (9200.0, -560.0, 390.0), (43.0, -18.0, 0.0), (1000.0, 430.0, 65.0))
    spawn_course_cube("Curved Slide Right 2", (10800.0, 560.0, 500.0), (43.0, 18.0, 0.0), (1000.0, 430.0, 65.0))
    spawn_course_cube("Curved Slide Left 3", (12400.0, -560.0, 610.0), (43.0, -18.0, 0.0), (1000.0, 430.0, 65.0))
    spawn_label("弯曲滑坡: 左右交替分段拼接", unreal.Vector(10800.0, 0.0, 820.0), 48.0)

    spawn_course_cube("Jump Ramp", (14200.0, 560.0, 660.0), (28.0, 18.0, 0.0), (650.0, 380.0, 75.0))
    spawn_label("跳台斜坡: BHop / 空中转向", unreal.Vector(14200.0, 1060.0, 900.0), 48.0)

    spawn_course_cube("Air Platform", (15600.0, 560.0, 800.0), (0.0, 0.0, 0.0), (720.0, 560.0, 70.0))
    spawn_label("空中平台", unreal.Vector(15600.0, 1060.0, 980.0), 48.0)

    boost_pad = spawn_script_actor("/Script/Momentum.ParkourBoostPad", "Boost Pad - Optional Trigger", (7400.0, -1120.0, 120.0))
    if boost_pad:
        boost_pad.set_editor_property("boost_strength", 1400.0)
        boost_pad.set_editor_property("vertical_boost", 80.0)
    spawn_label("Boost Pad: 独立触发器\n和真实加速坡分开放置", unreal.Vector(7400.0, -1540.0, 320.0), 46.0)

    finish = spawn_script_actor("/Script/Momentum.ParkourFinishVolume", "Finish Volume", (17000.0, 560.0, 940.0))
    set_first_box_extent(finish, (160.0, 240.0, 240.0))
    spawn_label("Finish / 终点", unreal.Vector(17000.0, 1060.0, 1180.0), 56.0)

    respawn = spawn_script_actor("/Script/Momentum.ParkourRespawnVolume", "Respawn Volume", (8500.0, 0.0, -900.0))
    set_first_box_extent(respawn, (19000.0, 5200.0, 160.0))

    spawn_script_actor("/Script/Momentum.ParkourBuildManager", "Runtime Build Manager / 开发期搭建管理器", (0.0, 900.0, 120.0))
    unreal.EditorLoadingAndSavingUtils.save_current_level()


ensure_dir(MAP_DIR)
create_main_menu()
create_test_level()
unreal.log("Momentum maps created: /Game/Maps/MainMenu and /Game/Maps/TestLevel")
