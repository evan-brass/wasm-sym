(module
  (type (;0;) (func (param i32 i32) (result i32)))
  (func (;0;) (type 0) (param i32 i32) (result i32)
    (local i32 i32)
    get_local 0
    get_local 1
    i32.const 4
    i32.mul
    i32.add
    set_local 2
    block  ;; label = @1
      loop  ;; label = @2
        get_local 0
        get_local 2
        i32.eq
        br_if 1 (;@1;)
        get_local 3
        get_local 0
        i32.load
        i32.add
        set_local 3
        get_local 0
        i32.const 4
        i32.add
        set_local 0
        br 0 (;@2;)
      end
    end
    get_local 3)
  (memory (;0;) 1)
  (export "memory" (memory 0))
  (export "accumulate" (func 0)))
