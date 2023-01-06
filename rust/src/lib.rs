#![feature(vec_into_raw_parts)]
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

pub fn vec_as_sqlite_vector(v: Vec<f32>) -> VectorFloat {
    let (data, size, _) = v.into_raw_parts();
    VectorFloat {
        data,
        size: size.try_into().unwrap(),
    }
}
