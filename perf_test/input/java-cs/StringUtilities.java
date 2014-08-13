/**
The StringUtilities class is a set of utility methods for dealing with String data
@author Aaron Moss (3217641)
*/
import java.util.*;

public class StringUtilities
{
	/**
	Counts the words (sets of non-blank characters separated by blanks) of a given string
	@param s	The string being considered
	@return The number of words
	*/
	public static int countWords(String s)
	{
		StringTokenizer tok = new StringTokenizer(s, " ");
		return tok.countTokens();
	} //end countWords method
	
	/**
	Counts the non-blank characters in the String
	@param s	The string being considered
	@return The number of non-blank characters
	*/
	public static int countNonBlanks(String s)
	{
		int count = 0;
		for(int i=0; i<s.length(); i++)
		{
			if (s.charAt(i) != ' ')
				count++;
		}
		return count;
	} //end countNonBlanks method
	
	/**
	Removes leading and trailing blanks from a string
	@param s	The string being worked on
	@return The string with leading and trailing blanks removed
	*/
	public static String removeLeadingAndTrailingBlanks(String s)
	{
		int length = s.length();
		//next two statements initialize index positions to easy to check "wrong" values
		int firstIndex = length;
		int lastIndex = -1;
		boolean done = false;
		//finds index of first nonblank character
		for(int i = 0; (i < length)&&(done == false); i++)
		{
			if (s.charAt(i) != ' ')
			{
				firstIndex = i;
				done = true; //exits for loop once first nonblank found
			}
		}
		//finds index of last nonblank character
		done = false;
		for(int i = (length-1); (i >= firstIndex)&&(done == false); i--)
		{
			if (s.charAt(i) != ' ')
			{
				lastIndex = i;
				done = true; //exits for loop once last nonblank found
			}
		}
		//checks to see whether the "wrong" values have been replaced
		if (firstIndex <= lastIndex)
			//makes sure to account for substring()'s extra character at the end
			return s.substring(firstIndex, lastIndex + 1);
		else
			return "";
	} //end removeLeadingAndTrailingBlanks method
	
	/**
	@param s1	The string to be searched
	@param s2	The substring to be found in s1
	@return	The index of the first occurence of s2 in s1
			-1 if there is no such occurance
	*/
	public static int indexOf(String s1, String s2)
	{
		/*initializes a few useful variables - lastIndex is the farthest we can push s2 along s1 
		before it "falls of the end" of s1*/
		int lastIndex = (s1.length() - s2.length());
		int s2Length = s2.length();
		//checks that s2 is shorter than s1
		if (lastIndex >= 0)
			{
				//loops through the string checking each possible position of s1 against s2
				for (int i = 0; i <= lastIndex; i++)
				{
					//this substring would be the substring the length of s2 that begins at i
					if(s2.equals(s1.substring(i, i + s2Length)))
						return i;
				}
				//if the for loop doesn't find a valid position, there isn't one, thus we return -1
				return -1;
			}
		else
			return -1;
	}
} //end StringUtilities class