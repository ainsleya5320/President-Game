import unreal as u,time
refresh_start=time.monotonic()
def refresh_view(dt):
 u.get_editor_subsystem(u.LevelEditorSubsystem).editor_invalidate_viewports()
 if time.monotonic()-refresh_start>25:u.unregister_slate_post_tick_callback(refresh_handle)
refresh_handle=u.register_slate_post_tick_callback(refresh_view)
