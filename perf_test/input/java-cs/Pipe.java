/**
  The Pipe class creates a pipe object for use in other programs
  @author Aaron Moss
*/

public class Pipe
{
	private double length;		//The length of the pipe
	private double innerDiam;	//The inner diameter of the pipe
	private double outerDiam;	//The outer diameter of the pipe
	
	/**
	Constructs a pipe using the input parameters
	@param lengthIn		The length of the pipe (double)
	@param innerDiamIn	The inner diameter of the pipe (double)
	@param outerDiamIn	The outer diameter of the pipe (double)
	*/
	public Pipe (double lengthIn,
					 double innerDiamIn,
					 double outerDiamIn)
	{
		length		=	lengthIn;
		innerDiam	=	innerDiamIn;
		outerDiam	=	outerDiamIn;
	} //end constructor
	
	/**
	Returns the length of the pipe
	@return The pipe length (double)
	*/
	public double getLength()
	{
		return length;
	} //end getLength
	
	/**
	Returns the inner diameter of the pipe
	@return The pipe's inner diameter (double)
	*/
	public double getInnerDiam()
	{
		return innerDiam;
	} //end getInnerDiam
	
	/**
	Returns the outer diameter of the pipe
	@return The pipe's outer diameter (double)
	*/
	public double getOuterDiam()
	{
		return outerDiam;
	} //end getOuterDiam
	
	/**
	Calculates and returns the volume of space occupied by the pipe
	@return The volume of the pipe (double)
	*/
	public double getVolume()
	{
		/*calculates the volume by calculating the difference between the areas of the outer circle of the pipe and the inner
		circle, then multiplying by the length - formula used is pi * length * ((outer_diameter^2 - inner_diameter^2)/4) */
		double volume = Math.PI * length * ((Math.pow(outerDiam, 2.0) - Math.pow(innerDiam, 2.0))/4.0);
		return volume;
	} //end getVolume
	
	/**
	Can set or change the length of the pipe
	@param lengthIn		The new length of the pipe (double)
	*/
	public void setLength(double lengthIn)
	{
	length = lengthIn;
	} //end setLength
	
	/**
	Can set or change the inner diameter of the pipe
	@param innerDiamIn	The new inner diameter of the pipe (double)
	*/
	public void setInnerDiam(double innerDiamIn)
	{
	innerDiam = innerDiamIn;
	} //end setInnerDiam
	
	/**
	Can set or change the outer diameter of the pipe
	@param outerDiamIn	The new outer diameter of the pipe (double)
	*/
	public void setOuterDiam(double outerDiamIn)
	{
	outerDiam = outerDiamIn;
	} //end setOuterDiam
} //End Pipe class

