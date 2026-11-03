program FunctionExpressions;
var
  a, b, result: integer;

function combine(x, y: integer): integer;
var
  temp: integer;
begin
  temp := x + y * 2;
  combine := (temp + 1) / 3;
end combine;

function add(x, y: integer): integer;
begin
  add := x + y;
end add;

begin
  a := 20;
  b := 6;

  result := combine(a, b);
  write(result);

  result := add(a + 1, b * 2);
  write(result);

  result := add(a, b) * 2;
  write(result);
end.
