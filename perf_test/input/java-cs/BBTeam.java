/**
The BBTeam class describes a basketball team, including the individual player objects
@author Aaron Moss 3217641
*/
public class BBTeam
{
	private final int MAXPLAYERS = 30; //the maximum number of players that can be on the team
	private BBPlayer[] team; //an array to hold the team members
	private String name; //the name of the team
	private int numPlayer; //the number of players on the team
	private int games; //the number of games the team has played
	
	/**
	@param nameIn	the name of the team
	@param gamesIn	the number of games played to date
	*/
	public BBTeam(String nameIn, int gamesIn)
	{
		team = new BBPlayer[MAXPLAYERS]; //initializes the team array to one of the maximum number of players allowed
		name = nameIn;
		games = gamesIn;
		numPlayer = 0; //sets the player counter
	} //end constructor
	
	/**
	Adds a player to the team
	@param playerIn	the player to be added
	*/
	public void addPlayer(BBPlayer playerIn)
	{
		if(numPlayer < MAXPLAYERS)
		{
			team[numPlayer] = playerIn;
			numPlayer++;
		}
		else
			System.out.println("Team Full -- Cannot Add Player " + numPlayer);
	} //end addPlayer method
	
	/**	@return a string with a list of active players */
	public String listActive()
	{
		String list = "\n" +
						  "Active Players on " + name + ":";
		for(int i = 0; i < numPlayer; i++)
		{
			if(team[i].getActive())
				list = list + "\n" + team[i].getName();
		} //end for
		return list;
	} //end listActive method
	
	/**	@return a string with a list of inactive players */
	public String listInactive()
	{
		String list = "\n" +
						  "Inactive Players on " + name + ":";
		for(int i = 0; i < numPlayer; i++)
		{
			if(!(team[i].getActive()))
				list = list + "\n" + team[i].getName();
		} //end for
		return list;
	} //end listInactive method
	
	/** @return a string with each player's points per game */
	public String listPlayerPPG()
	{
		String list = "\n" +
						  "Points per game by player (rounded to two decimal places):";
		for(int i = 0; i < numPlayer; i++)
		{
			list = list + "\n" + team[i].getName() + ": " + team[i].getPPG(); 
		} //end for
		return list;
	} //end listPlayerPPG method
	
	/** @return a string listing each player's fouls per game */
	public String listPlayerFPG()
	{
		String list = "\n" +
						  "Fouls per game by player (rounded to two decimal places):";
		for(int i = 0; i < numPlayer; i++)
		{
			list = list + "\n" + team[i].getName() + ": " + team[i].getFPG(); 
		} //end for
		return list;
	} //end listPlayerFPG method
	
	/** @return a string listing the total points scored by the team */
	public String getTotalPoints()
	{
		int tPoints = 0; //initializes a variable for total team points
		for(int i = 0; i < numPlayer; i++)
		{
			tPoints += team[i].getPoints();
		} //end for
		String list = "\n" + "Total Points for " + name + ": " + tPoints;
		return list;
	} //end getTotalPoints method
	
	/** @return a string listing the average points per game for the team, rounded to two decimal places */
	public String getTeamPPG()
	{
		int tPoints = 0; //initializes a variable for total team points
		for(int i = 0; i < numPlayer; i++)
		{
			tPoints += team[i].getPoints();
		} //end for
		double tPPG = (double)tPoints / games; //the true team points per game (the double cast is so the division of two int values isn't truncated)
		double rPPG = ((int)((tPPG * 100.0) + 0.5))/100.0; //the rounded team points per game
		String list = "\n" + "Average Points per Game for " + name + ": " + rPPG;
		return list;
	} //end getTeamPPG method
	
	/** @return a string listing the average fouls per game for the team */
	public String getTeamFPG()
	{
		int tFouls = 0; //initializes a variable for total team fouls
		for(int i = 0; i < numPlayer; i++)
		{
			tFouls += team[i].getFouls();
		} //end for
		double tFPG = (double)tFouls / games; //the true team fouls per game (the double cast is so the division of two int values isn't truncated)
		double rFPG = ((int)((tFPG * 100.0) + 0.5))/100.0; //the rounded team fouls per game
		String list = "\n" + "Average Fouls per Game for " + name + ": " + rFPG;
		return list;
	} //end getTeamFPG method
} //end BBTeam class