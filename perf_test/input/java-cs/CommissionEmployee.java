/**
The CommissonEmployee class is an Employee paid on commisson
@author Aaron Moss
*/

public class CommissionEmployee extends Employee
{
	private int amtSold; //the amount (in cents) the employee sold this week
	private int salesTarget; //the weekly sales target (in cents) for the employee
	private double commRate; //the commission rate the employee recieves
	private final double TENPERCENT = 0.1; //10%
	
	/**
	@param eIdIn			the employee ID
	@param fNameIn			the employee's first name
	@param lNameIn			the employee's last name
	@param amtSoldIn		the amount of (in cents) that the employee sold this week
	@param salesTargetIn		the employee's sales target
	@param commRateIn		the employee's commission rate
	*/
	public CommissionEmployee(int eIdIn,
									  String fNameIn,
									  String lNameIn,
									  int amtSoldIn,
									  int salesTargetIn,
									  double commRateIn)
	{
		super(eIdIn, fNameIn, lNameIn);
		amtSold = amtSoldIn;
		salesTarget = salesTargetIn;
		commRate = commRateIn;
	} //end constructor
	
	/**
	@return The weekly pay (in cents)
	*/
	public int weeklyPay()
	{
		//caluculates the base commison
		double pay = commRate * amtSold;
		if(amtSold > salesTarget)
			//if the employee did sell over target, adds an extra 10% of commission on the excess
			pay += (TENPERCENT*commRate*(amtSold-salesTarget));
		//casts the pay to int (with an extra 0.5 for correct rounding) and returns
		return (int)(pay + 0.5);
	} //end weeklyPay() method
} //end CommissionEmployee class