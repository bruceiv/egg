/**
The IntInitclass is a companion class to A3Q1, whose purpose is to create a random set of integers to be merged by this class
@author Aaron Moss
*/
import java.io.*;

public class IntInit {
	/**
	Creates a file of positive int values, terminated by -1
	@param args	A list of filenames to be set up as data files. Should be separated by spaces. If files previously exist, will be overwritten
	*/
	public static void main(String[] args) {
		final int NUM_INTS = 5; //a constant for the number of ints in the file
		final int MAX_START = 5; //a constant for the starting value in the file - must be >=0
		final int MAX_INCREMENT = 3; //a constant for the greatest spacing between the ordered variables - must be >=0
		
		for(int i = 0; i < args.length; i++) {
			try {
			DataOutputStream file = new DataOutputStream(new FileOutputStream(args[i])); //initializes the file to be written
			
			//fills the file with values
			int lastVal = (int)((Math.random() * MAX_START) + 1); //sets the first value to be written to the file
			for(int j = 1; j <= NUM_INTS; j++) {
				file.writeInt(lastVal); //writes current value
				lastVal += (int)((Math.random() * MAX_INCREMENT) + 1); //sets new write value, higher than the last
			} //inner for
			file.writeInt(-1); //terminiates the file with the sentinal value
			file.close();
			} catch(FileNotFoundException e){
			System.out.println("FILE NOT FOUND: " + e.getMessage());
			System.exit(0);
			} catch(IOException e) {
			System.out.println(e.getMessage());
			System.exit(0);
			}
		} //outer for
	} //main()
} //IntInit class

