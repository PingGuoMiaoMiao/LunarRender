///|
fn empty_pmx() -> Bytes {
  Bytes::from_array([
    b'P', b'M', b'X', b' ', 0, 0, 0, 64, 8, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,
  ])
}

///|
test "PMX parser accepts empty model" {
  inspect(empty_pmx().length(), content="49")
  let model = parse(empty_pmx())
  match model {
    None => panic()
    Some(model) => {
      inspect(model.vertex_count, content="0")
      inspect(model.index_count, content="0")
      inspect(model.texture_count, content="0")
      inspect(model.render_vertex_count(), content="0")
    }
  }
}

///|
test "PMX parser rejects invalid header" {
  let bytes = Bytes::from_array([b'N', b'O', b'P', b'E'])
  match parse(bytes) {
    None => println("invalid header rejected")
    Some(_) => panic()
  }
}

///|
test "PMX parser rejects truncation" {
  let bytes = Bytes::from_array(empty_pmx()[:20].to_array())
  match parse(bytes) {
    None => println("truncation rejected")
    Some(_) => panic()
  }
}

///|
test "PMX parser keeps high 16-bit vertex indices unsigned" {
  let data = empty_pmx()[:37].to_array()
  data[11] = 2
  data.push(1)
  data.push(0)
  data.push(0)
  data.push(0)
  data.push(0)
  data.push(128)
  for _ in 0..<8 {
    data.push(0)
  }
  match parse(Bytes::from_array(data)) {
    None => panic()
    Some(model) => {
      inspect(model.index_count, content="1")
      inspect(model.indices[0], content="32768")
    }
  }
}
