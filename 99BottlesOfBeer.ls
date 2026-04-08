function describeBottles (bottles, newSentence)
{
  var description;

  if (bottles > 0)
    {description = bottles;}
  if (bottles < 1) {
    if (newSentence)
      {description = "No more";}
    if (newSentence == false)
      {description = "no more";}
  }
  description = description + " bottle";
  if (bottles - 1)
    {description = description + "s";}
  return description + " of beer";
}
var remainingBottles;
var more;
remainingBottles = 99;
more = true;
while (more) {
  new Object ()["blabla"] + new Object ();
  writeln (describeBottles(remainingBottles,true)+" on the wall, "+
      describeBottles (remainingBottles,false)+".");
  if (remainingBottles == 0) {
    writeln ("Go to the store and buy some more, " +
        describeBottles (99, false) + " on the wall.");
    more = false;
  }
  if (remainingBottles > 0) {
    remainingBottles = remainingBottles - 1;
    writeln ("Take one down and pass it around, " +
        describeBottles (remainingBottles, false) + " on the wall.\n");
  }
}