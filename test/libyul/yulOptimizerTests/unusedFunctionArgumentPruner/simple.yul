{
    let z := f(1)
    function f(x) -> y { }
}
// ----
// step: unusedFunctionArgumentPruner
//
// {
//     let z := f_1(1)
//     function f() -> y
//     { }
//     function f_1(x) -> y
//     { y := f() }
// }
