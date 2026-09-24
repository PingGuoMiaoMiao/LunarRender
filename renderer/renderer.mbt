///|
pub(all) struct Renderer {
  mut count : Int
}

///|
pub fn Renderer::new() -> Renderer {
  { count: 0, }
}

///|
pub fn Renderer::vertex_count(self : Renderer) -> Int {
  self.count
}

///|
pub fn Renderer::upload(self : Renderer, mesh : @mesh.ChunkMeshData) -> Bool {
  let uploaded = @native.gl_upload_mesh(mesh.vertices, mesh.vertex_count)
  if uploaded {
    self.count = mesh.vertex_count
  }
  uploaded
}

///|
pub fn Renderer::upload_chunk(
  self : Renderer,
  position : @world.ChunkPos,
  mesh : @mesh.ChunkMeshData,
) -> Bool {
  let uploaded = @native.gl_upload_chunk_mesh(
    position.x,
    position.z,
    mesh.vertices,
    mesh.vertex_count,
    mesh.opaque_vertex_count,
    mesh.cutout_vertex_count,
    mesh.translucent_vertex_count,
  )
  if uploaded {
    self.count = self.count + mesh.vertex_count
  }
  uploaded
}

///|
pub fn Renderer::remove_chunk(_self : Renderer, position : @world.ChunkPos) -> Unit {
  @native.gl_remove_chunk_mesh(position.x, position.z)
}

///|
pub fn Renderer::draw_chunks(
  _self : Renderer,
  view_projection : FixedArray[Float],
) -> Unit {
  @native.gl_draw_chunk_meshes(view_projection)
}

///|
pub fn Renderer::upload_player(
  _self : Renderer,
  mesh : @mesh.PlayerMeshData,
) -> Bool {
  @native.gl_upload_player_mesh(mesh.vertices, mesh.vertex_count)
}

///|
pub fn Renderer::draw_player(
  _self : Renderer,
  view_projection : FixedArray[Float],
) -> Unit {
  @native.gl_draw_player_mesh(view_projection)
}

///|
pub fn Renderer::draw_block_outline(
  _self : Renderer,
  view_projection : FixedArray[Float],
  block_x : Int,
  block_y : Int,
  block_z : Int,
) -> Unit {
  @native.gl_draw_block_outline(view_projection, block_x, block_y, block_z)
}

///|
pub fn Renderer::draw(
  self : Renderer,
  view_projection : FixedArray[Float],
) -> Unit {
  @native.gl_draw_mesh(view_projection, self.count)
}

///|
pub fn Renderer::draw_frame(
  _self : Renderer,
  snapshot : @render.RenderSnapshot,
) -> Unit {
  @native.gl_draw_chunk_meshes(snapshot.view_projection)
  if snapshot.show_player {
    @native.gl_draw_player_mesh(snapshot.view_projection)
  }
  match snapshot.selection {
    Some(selection) => @native.gl_draw_block_outline(
      snapshot.view_projection,
      selection.block_x,
      selection.block_y,
      selection.block_z,
    )
    None => ()
  }
  @native.gl_draw_hotbar(snapshot.hud.selected_slot, 960, 540)
  @native.gl_draw_command_overlay(
    snapshot.hud.command_open,
    snapshot.hud.command_line,
    snapshot.hud.command_status,
    960,
    540,
  )
}

///|
pub fn Renderer::shutdown(_self : Renderer) -> Unit {
  @native.gl_destroy_renderer()
}
