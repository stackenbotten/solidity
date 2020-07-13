{
	function f(a, b, c) -> x, y, z
	{
		x, y, z := f(1, 2, 3)
	}
}
// ----
// step: unusedFunctionArgumentPruner
//
// {
//     function f() -> x, y, z
//     { x, y, z := f_1(1, 2, 3) }
//     function f_1(a, b, c) -> x, y, z
//     { x, y, z := f() }
// }
