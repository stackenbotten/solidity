{
	function f(a) -> x { x := a }
	function g(b) -> y { pop(g(b)) }
}
// ----
// step: unusedFunctionArgumentPruner
//
// {
//     function f(a) -> x
//     { x := a }
//     function g(b) -> y
//     { pop(g(b)) }
// }
