/* Defines Triangle class
   Created: Sept. 23, 2006
   Author: Aaron Moss
*/

public class Triangle
{  //stores values for sides a, b, and c
   private double a;
   private double b;
   private double c;

   //constructor method
   public Triangle(double aIn,
		   double bIn,
		   double cIn)
   {
    a = aIn;
    b = bIn;
    c = cIn;
   }  //end constructor

   //calculates perimeter (a+b+c) and returns as double
   public double calcPerimeter()
   {
    double perimeter = a+b+c;
    return(perimeter);
   }  //end calcPerimeter

   //calculates area by Heron's formula:
   //(sqrt(s(s-a)(s-b)(s-c)) where s=(a+b+c)/2)
   //returns as double
   public double calcArea()
   {
    double s		= (a+b+c)/2;
    double underRadical	= s*(s-a)*(s-b)*(s-c);
    double area		= Math.sqrt(underRadical);
    return(area);
   }  //end calcArea

}  //end Triangle class definition