(module
 (export "test" (func $test))
 (func $test (param $x i32) (param $y i32) (param $z i32)
  (if (i32.eq (i32.add (get_local $x) (get_local $y)) (i32.const 42))
   (unreachable)
  )
 )
)
