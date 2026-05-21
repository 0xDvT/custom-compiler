program p3;

var
  a, b, c: integer;
  abcd: string;

function addtwice(x, y: integer; z: integer): integer;
var
  t: integer;
begin
  t := x + y;
  addtwice := t + z;
end addtwice;

begin
  a := 5;
  b := 7;
  c := (addtwice(a, b+1, 3) * 2 + 1) / (1+2);
  write(c);

end.