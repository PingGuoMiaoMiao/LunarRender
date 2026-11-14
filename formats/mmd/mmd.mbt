///|
#derive(Debug)
pub(all) struct PmxMaterial {
  texture_index : Int
  first_index : Int
  index_count : Int
  diffuse_r : Float
  diffuse_g : Float
  diffuse_b : Float
  diffuse_a : Float
}

///|
pub extend PmxMaterial with @debug.Debug::{to_repr}

///|
#derive(Debug)
pub(all) struct PmxModel {
  vertex_count : Int
  index_count : Int
  texture_count : Int
  positions_normals_uv : FixedArray[Float]
  indices : FixedArray[Int]
  materials : Array[PmxMaterial]
}

///|
pub extend PmxModel with @debug.Debug::{to_repr}

///|
const MmdAtlasTileSize : Double = 512.0

///|
const MmdAtlasTileSizeMinusOne : Double = 511.0

///|
const MmdAtlasWidth : Double = 2048.0

///|
const MmdAtlasHeight : Double = 1536.0

///|
priv struct Cursor {
  bytes : Bytes
  mut offset : Int
  mut valid : Bool
  mut additional_vec4_count : Int
  mut vertex_index_size : Int
  mut texture_index_size : Int
  mut bone_index_size : Int
}

///|
fn Cursor::ensure(self : Cursor, size : Int) -> Bool {
  if size < 0 || self.offset < 0 || self.offset > self.bytes.length() - size {
    self.valid = false
    return false
  }
  true
}

///|
fn Cursor::read_u8(self : Cursor) -> Int {
  if !self.ensure(1) {
    return 0
  }
  let value = self.bytes[self.offset].to_int()
  self.offset += 1
  value
}

///|
fn Cursor::read_i16(self : Cursor) -> Int {
  if !self.ensure(2) {
    return 0
  }
  let value = self.bytes.unsafe_read_uint16_le(self.offset).to_int()
  self.offset += 2
  if value >= 32768 {
    value - 65536
  } else {
    value
  }
}

///|
fn Cursor::read_i32(self : Cursor) -> Int {
  if !self.ensure(4) {
    return 0
  }
  let value = self.bytes.unsafe_read_uint32_le(self.offset).reinterpret_as_int()
  self.offset += 4
  value
}

///|
fn Cursor::read_f32(self : Cursor) -> Float {
  if !self.ensure(4) {
    return 0.0
  }
  let value = Float::reinterpret_from_uint(
    self.bytes.unsafe_read_uint32_le(self.offset),
  )
  self.offset += 4
  value
}

///|
fn Cursor::read_index(self : Cursor, size : Int) -> Int {
  match size {
    1 => {
      let value = self.read_u8()
      if value >= 128 {
        value - 256
      } else {
        value
      }
    }
    2 => self.read_i16()
    4 => self.read_i32()
    _ => {
      self.valid = false
      0
    }
  }
}

///|
fn Cursor::read_vertex_index(self : Cursor, size : Int) -> Int {
  match size {
    1 => self.read_u8()
    2 => {
      if !self.ensure(2) {
        return 0
      }
      let value = self.bytes.unsafe_read_uint16_le(self.offset).to_int()
      self.offset += 2
      value
    }
    4 => {
      if !self.ensure(4) {
        return 0
      }
      let value = self.bytes
        .unsafe_read_uint32_le(self.offset)
        .reinterpret_as_int()
      self.offset += 4
      value
    }
    _ => {
      self.valid = false
      0
    }
  }
}

///|
fn Cursor::skip(self : Cursor, size : Int) -> Bool {
  if !self.ensure(size) {
    return false
  }
  self.offset += size
  true
}

///|
fn Cursor::skip_string(self : Cursor) -> Bool {
  let length = self.read_i32()
  if !self.valid || length < 0 {
    self.valid = false
    return false
  }
  self.skip(length)
}

///|
fn skip_weight(cursor : Cursor, weight_type : Int) -> Bool {
  match weight_type {
    0 => cursor.skip(cursor.bone_index_size)
    1 => cursor.skip(cursor.bone_index_size * 2 + 4)
    2 => cursor.skip(cursor.bone_index_size * 4 + 16)
    3 => cursor.skip(cursor.bone_index_size * 2 + 4 + 36)
    4 => cursor.skip(cursor.bone_index_size * 4 + 16)
    _ => {
      cursor.valid = false
      false
    }
  }
}

///|
fn clamp_unit(value : Float) -> Float {
  if value < 0.0 {
    0.0
  } else if value > 1.0 {
    1.0
  } else {
    value
  }
}

///|
fn write_model_vertex(
  output : FixedArray[Float],
  base : Int,
  model : PmxModel,
  vertex_index : Int,
  material : PmxMaterial,
) -> Bool {
  if vertex_index < 0 || vertex_index >= model.vertex_count {
    return false
  }
  let source = vertex_index * 8
  let u = clamp_unit(model.positions_normals_uv[source + 6])
  let v = clamp_unit(model.positions_normals_uv[source + 7])
  let texture = if material.texture_index >= 0 &&
    material.texture_index < model.texture_count {
    material.texture_index
  } else {
    0
  }
  let tile_x = texture % 4
  let tile_y = texture / 4
  let atlas_u = (
      tile_x.to_double() * MmdAtlasTileSize +
      0.5 +
      u.to_double() * MmdAtlasTileSizeMinusOne
    ) /
    MmdAtlasWidth
  let atlas_v = (
      tile_y.to_double() * MmdAtlasTileSize +
      0.5 +
      v.to_double() * MmdAtlasTileSizeMinusOne
    ) /
    MmdAtlasHeight
  output[base] = model.positions_normals_uv[source]
  output[base + 1] = model.positions_normals_uv[source + 1]
  output[base + 2] = model.positions_normals_uv[source + 2]
  output[base + 3] = model.positions_normals_uv[source + 3]
  output[base + 4] = model.positions_normals_uv[source + 4]
  output[base + 5] = model.positions_normals_uv[source + 5]
  output[base + 6] = Float::from_double(atlas_u)
  output[base + 7] = Float::from_double(atlas_v)
  output[base + 8] = material.diffuse_r
  output[base + 9] = material.diffuse_g
  output[base + 10] = material.diffuse_b
  output[base + 11] = material.diffuse_a
  true
}

///|
/// Counts the vertices `render_vertices` would emit without building the
/// interleaved buffer, so the caller can size one allocation exactly.
fn render_vertex_slots(model : PmxModel) -> Int {
  let mut total = 0
  let index_length = model.indices.length()
  for material in model.materials {
    let start = material.first_index
    let end = if start + material.index_count > index_length {
      index_length
    } else {
      start + material.index_count
    }
    let mut i = start
    while i < end {
      let vertex_index = model.indices[i]
      if vertex_index < 0 || vertex_index >= model.vertex_count {
        return 0
      }
      total += 1
      i += 1
    }
  }
  total
}

///|
pub fn PmxModel::render_vertices(self : PmxModel) -> FixedArray[Float] {
  let slots = render_vertex_slots(self)
  let output : FixedArray[Float] = FixedArray::make(slots * 12, 0.0)
  if slots == 0 {
    return output
  }
  let mut cursor = 0
  for material in self.materials {
    let start = material.first_index
    let end = if start + material.index_count > self.indices.length() {
      self.indices.length()
    } else {
      start + material.index_count
    }
    let mut i = start
    while i < end {
      let vertex_index = self.indices[i]
      if !write_model_vertex(output, cursor, self, vertex_index, material) {
        return FixedArray::make(0, 0.0)
      }
      cursor += 12
      i += 1
    }
  }
  output
}

///|
pub fn PmxModel::render_vertex_count(self : PmxModel) -> Int {
  render_vertex_slots(self)
}

///|
pub fn PmxModel::render_vertices_normalized(
  self : PmxModel,
  target_x : Double,
  target_y : Double,
  target_z : Double,
  target_height : Double,
) -> FixedArray[Float] {
  let raw = self.render_vertices()
  if raw.length() == 0 {
    return raw
  }
  let mut min_x = raw[0].to_double()
  let mut max_x = min_x
  let mut min_y = raw[1].to_double()
  let mut max_y = min_y
  let mut min_z = raw[2].to_double()
  let mut max_z = min_z
  let mut i = 0
  while i < raw.length() {
    let x = raw[i].to_double()
    let y = raw[i + 1].to_double()
    let z = raw[i + 2].to_double()
    if x < min_x {
      min_x = x
    }
    if x > max_x {
      max_x = x
    }
    if y < min_y {
      min_y = y
    }
    if y > max_y {
      max_y = y
    }
    if z < min_z {
      min_z = z
    }
    if z > max_z {
      max_z = z
    }
    i += 12
  }
  let height = max_y - min_y
  if height <= 0.0 {
    return FixedArray::make(0, 0.0)
  }
  let scale = target_height / height
  let center_x = (min_x + max_x) / 2.0
  let center_z = (min_z + max_z) / 2.0
  FixedArray::makei(raw.length(), i => {
    let component = i % 12
    if component == 0 {
      Float::from_double((raw[i].to_double() - center_x) * scale + target_x)
    } else if component == 1 {
      Float::from_double((raw[i].to_double() - min_y) * scale + target_y)
    } else if component == 2 {
      Float::from_double((raw[i].to_double() - center_z) * scale + target_z)
    } else {
      raw[i]
    }
  })
}

///|
pub fn parse(bytes : Bytes) -> PmxModel? {
  if bytes.length() < 17 {
    return None
  }
  if bytes[0] != b'P' ||
    bytes[1] != b'M' ||
    bytes[2] != b'X' ||
    bytes[3] != b' ' {
    return None
  }
  let cursor = Cursor::{
    bytes,
    offset: 4,
    valid: true,
    additional_vec4_count: 0,
    vertex_index_size: 0,
    texture_index_size: 0,
    bone_index_size: 0,
  }
  ignore(cursor.read_f32())
  let data_size = cursor.read_u8()
  let encoding = cursor.read_u8()
  let additional_vec4_count = cursor.read_u8()
  let vertex_index_size = cursor.read_u8()
  let texture_index_size = cursor.read_u8()
  ignore(cursor.read_u8())
  let bone_index_size = cursor.read_u8()
  ignore(cursor.read_u8())
  ignore(cursor.read_u8())
  if !cursor.valid || data_size != 8 || (encoding != 0 && encoding != 1) {
    return None
  }
  cursor.additional_vec4_count = additional_vec4_count
  cursor.vertex_index_size = vertex_index_size
  cursor.texture_index_size = texture_index_size
  cursor.bone_index_size = bone_index_size
  if vertex_index_size != 1 && vertex_index_size != 2 && vertex_index_size != 4 {
    return None
  }
  if texture_index_size != 1 &&
    texture_index_size != 2 &&
    texture_index_size != 4 {
    return None
  }
  if bone_index_size != 1 && bone_index_size != 2 && bone_index_size != 4 {
    return None
  }
  for _ in 0..<4 {
    if !cursor.skip_string() {
      return None
    }
  }
  let vertex_count = cursor.read_i32()
  if !cursor.valid || vertex_count < 0 || vertex_count > 2000000 {
    return None
  }
  // Every PMX vertex stores 8 float32 values before its weight and edge data,
  // so a count larger than the file allows is rejected before allocating.
  if vertex_count > bytes.length() / 32 {
    return None
  }
  let positions_normals_uv : FixedArray[Float] = FixedArray::make(
    vertex_count * 8,
    0.0,
  )
  for vertex = 0; vertex < vertex_count; vertex = vertex + 1 {
    let x = cursor.read_f32()
    let y = cursor.read_f32()
    let z = cursor.read_f32()
    let normal_x = cursor.read_f32()
    let normal_y = cursor.read_f32()
    let normal_z = cursor.read_f32()
    let u = cursor.read_f32()
    let v = cursor.read_f32()
    if !cursor.skip(cursor.additional_vec4_count * 16) {
      return None
    }
    let weight_type = cursor.read_u8()
    if !skip_weight(cursor, weight_type) {
      return None
    }
    ignore(cursor.read_f32())
    let base = vertex * 8
    positions_normals_uv[base] = x
    positions_normals_uv[base + 1] = y
    positions_normals_uv[base + 2] = z
    positions_normals_uv[base + 3] = normal_x
    positions_normals_uv[base + 4] = normal_y
    positions_normals_uv[base + 5] = normal_z
    positions_normals_uv[base + 6] = u
    positions_normals_uv[base + 7] = v
  }
  let index_count = cursor.read_i32()
  if !cursor.valid || index_count < 0 || index_count > 6000000 {
    return None
  }
  if index_count * cursor.vertex_index_size > bytes.length() {
    return None
  }
  let indices : FixedArray[Int] = FixedArray::make(index_count, 0)
  for index = 0; index < index_count; index = index + 1 {
    indices[index] = cursor.read_vertex_index(cursor.vertex_index_size)
  }
  let texture_count = cursor.read_i32()
  if !cursor.valid || texture_count < 0 || texture_count > 4096 {
    return None
  }
  for _ in 0..<texture_count {
    if !cursor.skip_string() {
      return None
    }
  }
  let material_count = cursor.read_i32()
  if !cursor.valid || material_count < 0 || material_count > 4096 {
    return None
  }
  let materials : Array[PmxMaterial] = []
  let mut first_index = 0
  for _ in 0..<material_count {
    if !cursor.skip_string() || !cursor.skip_string() {
      return None
    }
    let diffuse_r = cursor.read_f32()
    let diffuse_g = cursor.read_f32()
    let diffuse_b = cursor.read_f32()
    let diffuse_a = cursor.read_f32()
    if !cursor.skip(12 + 4 + 12 + 1 + 16 + 4) {
      return None
    }
    let texture_index = cursor.read_index(cursor.texture_index_size)
    ignore(cursor.read_index(cursor.texture_index_size))
    ignore(cursor.read_u8())
    let toon_flag = cursor.read_u8()
    if toon_flag == 0 {
      ignore(cursor.read_index(cursor.texture_index_size))
    } else {
      ignore(cursor.read_u8())
    }
    if !cursor.skip_string() {
      return None
    }
    let surface_count = cursor.read_i32()
    if !cursor.valid ||
      surface_count < 0 ||
      first_index > index_count - surface_count {
      return None
    }
    materials.push({
      texture_index,
      first_index,
      index_count: surface_count,
      diffuse_r,
      diffuse_g,
      diffuse_b,
      diffuse_a,
    })
    first_index += surface_count
  }
  if !cursor.valid || first_index > index_count {
    return None
  }
  Some({
    vertex_count,
    index_count,
    texture_count,
    positions_normals_uv,
    indices,
    materials,
  })
}
