/**
The LineSegment class defines a line segment on a Cartesian plane, utilizing the Point class
@author Aaron Moss
*/
public class LineSegment
{
	private Point leftPoint;
	private Point rightPoint;
	
	/**
	Constructs a LineSegment object from its two endpoints - if the endpoints are the same it sets the line segment to be
	one unit long, with the given point as the left endpoint
	@param point1	One endpoint
	@param point2	The other endpoint
	*/
	public LineSegment(Point point1, Point point2)
	{
		if (!(point1.equals(point2)))
		{
			//checks to see whether point1 is to the left of point2 (has a lower x value)
			if(point1.getX() <= point2.getX())
			{
				leftPoint  = point1;
				rightPoint = point2;
			}
			else
			{
				leftPoint  = point2;
				rightPoint = point1;
			}
		}
		else
		{
			leftPoint  = point1;
			/*sets the right endpoint to a new point with the same values as point2 (which is the same as point1),
			except that the x value is one higher (one unit to the right)*/
			rightPoint = new Point((point2.getX() + 1.0), point2.getY());
		}
	} //end constructor
	
	/**
	@return The left endpoint
	*/
	public Point getLeftPoint()
	{return leftPoint;}
	
	/**
	@return The right endpoint
	*/
	public Point getRightPoint()
	{return rightPoint;}
	
	/**
	Sets a new left endpoint - if the supplied point is the same as the current right endpoint, it sets it to a point 1 unit
	directly to the left of the current right endpoint, otherwise the parameter is used as the new left endpoint
	@param pointIn	The new left endpoint
	*/
	public void setLeftPoint(Point pointIn)
	{
		if (!(pointIn.equals(rightPoint)))
			leftPoint = pointIn;
		else
			leftPoint = new Point((pointIn.getX() - 1.0), pointIn.getY());
	} //end setLeftPoint method
	
	/**
	Sets a new right endpoint - if the supplied point is the same as the current left endpoint, it sets it to a point 1 unit
	directly to the right of the current left endpoint, otherwise the parameter is used as the new right endpoint
	@param pointIn	The new right endpoint
	*/
	public void setRightPoint(Point pointIn)
	{
		if (!(pointIn.equals(leftPoint)))
			rightPoint = pointIn;
		else
			rightPoint = new Point((pointIn.getX() + 1.0), pointIn.getY());
	} //end setRightPoint method
	
	/**
	Checks whether a point is on the same line as the points of the line segment
	@param otherPoint	The point being checked
	@return Is the point on the line?
	*/
	public boolean isOnLine(Point otherPoint)
	{
		//checks if the provided point is an endpoint of the line
		if ((otherPoint.equals(leftPoint)) || (otherPoint.equals(rightPoint)))
			{return true;}
		else
		{
			//checks whether the provided point is on the same vertical line as both endpoints
			if (	(rightPoint.getX() == leftPoint.getX())
				&& (otherPoint.getX() == leftPoint.getX()))
				{return true;}
			else
			{
				double segmentSlope, testSlope;
				//finds the slope of the line segment using the formula slope = (y2 – y1) / (x2 – x1)
				segmentSlope = (  (rightPoint.getY() - leftPoint.getY())
									 / (rightPoint.getX() - leftPoint.getX()));
				/*finds the slope of the supplied point and the left endpoint
				(or, if the x is vertically aligned with the left endpoint, the right endpoint)*/
				if (otherPoint.getX() != leftPoint.getX())
				{
					testSlope = (  (otherPoint.getY() - leftPoint.getY())
									 / (otherPoint.getX() - leftPoint.getX()));
				}
				else
				{
					testSlope = (  (rightPoint.getY() - otherPoint.getY())
									 / (rightPoint.getX() - otherPoint.getX()));
				}
				
				/*checks if the slopes are equal - if they are, since they both use the same point (the left endpoint),
				the supplied point is on the same line as the line segment*/
				return (segmentSlope == testSlope);
			}
		}
	}
} //end LineSegment class