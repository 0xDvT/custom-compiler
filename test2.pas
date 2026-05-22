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
  write(addtwice(a, b+1, 3));
  c := (addtwice(addtwice(a, b+1, 3), b+1, 3));
  write(c);

end.