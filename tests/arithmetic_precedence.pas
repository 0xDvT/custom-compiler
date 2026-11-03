program ArithmeticPrecedence;
var
  a, b, c, result: integer;
begin
  a := 20;
  b := 6;
  c := 4;

  result := a + b * c;
  write(result);

  result := (a + b) / c;
  write(result);

  result := a - b - c;
  write(result);

  result := a / b;
  write(result);
end.
