/**
CS 1083 Asst. 1 - Q3a
The distance class measures holds distances between cities
@author Aaron Moss
*/
public class Distance
{
	private String[] city = {"Atlanta", "Chicago", "Denver", "Houston", "Kansas"}; //the array to hold the city names - note alphabetical arrangement

	private int[] atlanta = {}; //distances from Atlanta
	private int[] chicago = {1150}; //distances from Chicago
	private int[] denver = {2260, 1615}; //distances from Denver
	private int[] houston = {1285, 1750, 1805}; //distances from Houston
	private int[] kansas = {1295, 850, 965, 1280}; //distances from kansas
	
	private int[][] distance = {atlanta,
										 chicago,
										 denver,
										 houston,
										 kansas};
	//note that the city in distance[i] is the same as the city in city[i]
	
	/** Constuctor needs to be run before class can be utilized */
	 public Distance() {}

	/**
	@param index	The array index
	@return			The city asociated with the index
					In case of invalid index, returns "-1"
	*/
	public String returnCity(int index)
	{
		if ((index < distance.length) && (index >= 0)) //checks for a valid index value
			{return city[index];}
		else
			{return "-1";} //unlikely a city will ever be named "-1"
	} //returnCity()
	
	/**
	This method finds the distance between two cities, using their index numbers in the array
	@param city1	The array index of one city
	@param city2	The array index of the other city
	@return			The distance (in km)
					In case of invalid index, returns -1
	*/
	public int findDistance(int city1, int city2)
	{
		if ((city1 < distance.length) && (city2 < distance.length) && (city1 >= 0) && (city2 >= 0)) //checks for valid indexes
		{
			if (city1 > city2)
			{
				return distance[city1][city2];
				//this is the distance between the two cities, which is found in the array for the city farther down the alphabet (greater array index)
			} //if
			else if (city1 < city2)
			{	return distance[city2][city1];} //same concept as above
			else
			{	return 0;} //same city, zero distance
		} //valid city distances
		else
		{	return -1;}
	} //findDistance (array index version)
	
	/**
	This method finds the closest city to a given city, given its array index
	Imperitive that there be at least two cities in the data set (a reasonable assumption)
	@param city		The index of the city we are considering
	@return			The array index of the city that is closest
					-1 in case of invalid index
	*/
	public int findClosest(int city)
	{
		if ((city < distance.length) && (city >= 0)) //checks for valid index
		{
			int close; //the array index of the closest city
			int lowVal; //the shortest distance yet found
			
			if (city >= 1) //checks if we can put close in as zero safely 
			{
				close = 0;
				lowVal = distance[city][close];
			} //if
			else if (city == 0) //if the given city is index 0, sets up initial values
			{
				close = 1;
				lowVal = distance[close][city];
			} //else if
			else //shouldn't run - precluded by upper if
			{	return -1;}
			
			for (int i = 0; i<city; i++) //checks the cities whose distances are stored in the array belonging to the city being considered
			{
				if (distance[city][i] < lowVal) //checks to see if the city with index i is closer than the current closest city
				{
					close = i;
					lowVal = distance[city][i];
				} //if
			} //for(1)
			
			for (int i = (city + 1); i<distance.length; i++) //checks the cities who store the distance to the city being considered in their own arrays
			{
				if (distance[i][city] < lowVal) //checks to see whether the city with index i is closer than the current closest city
				{
					close = i;
					lowVal = distance[i][city];
				} //if
			} //for(2)
			
			return close; //returns the closest city
		} //valid index values
		else
		{	return -1;}
	} //findClosest()
	
	/**
	This method uses binary sort to determine what the index of a given city is 
	It is therefore dependant on the cities being arranged in alphabetical order in their array
	@param name	The name of the city (not case sensitive)
	@return			The city's array index
					-1 if city not found
	*/
	public int returnIndex(String name)
	{
		int low = 0; //lowest value in plausible range
		int high = (city.length - 1); //highest value in plausible range
		while (low <= high) //while there are values in the plausible range
		{
			int mid = (low + high)/2; //current check value
			if (name.compareToIgnoreCase(city[mid]) == 0) //checks if the supplied name is the name currently being checked
			{	return mid;}
			else if (name.compareToIgnoreCase(city[mid]) > 0) //checks if the supplied name is lower in the alphabet than the name currently being checked
			{	low = mid + 1;}
			else //executes if the supplied name is higher in the alphabet than the name currently being checked
			{	high = mid - 1;}
		} //while
		return -1; //if this is reached, the specified city name has not been found
	} // returnIndex()
} //Distance class