/**
The CalculatorGUI class provides a GUI for the Calculator class
@author Aaron Moss
*/
import javax.swing.*;
import java.awt.*;
import java.awt.event.*;

public class CalculatorGUI extends JFrame
									implements ActionListener {
	private JButton clear = new JButton("C"); //the clear button
	private JButton allClear = new JButton("AC"); //the all clear button
	private JTextField screen; //the screen
	private JButton[] numkeys; //the rest of the buttons (numbers and operators)
	private String[] labels = {"7", "8", "9", "+",
										"4", "5", "6", "-",
										"1", "2", "3", "*",
										"0", "S", "/", "="}; //the button labels
	private JPanel inPanel; //the keypad's panel
	private JPanel outPanel; //for the screen and the clear button
	
	char lastOp = '='; //the last operator used, initialized to equals (for no operator used)
	int lastTerm = 0; //the last arithmetic term entered
	int scrVal = 0; //integer value of the current display
	boolean overwrite = false; //overwrite the current screen value?
	
	public CalculatorGUI() {
		//sets up window
		Container masterPane = getContentPane();
		masterPane.setLayout(new BorderLayout());
		this.setDefaultCloseOperation(EXIT_ON_CLOSE);
		
		//sets up screen and clear button
		outPanel = new JPanel();
		outPanel.setLayout(new BoxLayout(outPanel, BoxLayout.X_AXIS));
		clear.addActionListener(this);
		allClear.addActionListener(this);
		screen = new JTextField("0", 20);
		screen.setEditable(false);
		outPanel.add(screen);
		outPanel.add(clear);
		outPanel.add(allClear);
		
		//sets up keypad
		inPanel = new JPanel(new GridLayout(4, 4, 1, 1));
		numkeys = new JButton[labels.length];
		for(int i = 0; i<numkeys.length; i++) { //initializes keypad buttons
			numkeys[i] = new JButton(labels[i]);
			numkeys[i].addActionListener(this);
			inPanel.add(numkeys[i]);
		} //for - key initializer
		
		//adds elements to pane
		masterPane.add("North", outPanel);
		masterPane.add("Center", inPanel);
		
		//finishes window
		setTitle("Calculator");
		pack();
		setVisible(true);
	} //constructor
	
	public void actionPerformed(ActionEvent e) {
		try {
		char keyPress = (((JButton)e.getSource()).getText()).charAt(0); //the label on the key pressed
		buttonPick(keyPress);
		} catch (Exception ex) {
			scrVal = 0;
			screen.setText("CRITICAL ERROR");
		}
	} //actionPerformed()
	
	/**
	Runs an action signified by a given character
	@param keyPress	The key pressed
	*/
	public void buttonPick(char keyPress) {
		try {
		switch (keyPress) {
			//if  a number key is pressed
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				addnum(keyPress); //adds current number to screen
				break;
			
			//if an operator is pressed
			case '+':
			case '-':
			case '*':
			case '/':
				newOp(keyPress); //runs the operator code
				break;
			case '=':
				if (lastOp != '=')
					comp(); //computes the last expression
				break;
			
			//other keys
			case 'A': //the all clear button
				lastOp = '=';
				lastTerm = '0';
			case 'C': //the clear button
				scrVal = 0; //zeros the current screen value
				screen.setText("0"); //displays onscreen
				break;
			case 'S': //the sign switching button
				scrVal = (-1 * scrVal); //negates current value
				screen.setText(Integer.toString(scrVal)); //displays onscreen
				break;
		} //switch - button pressed
		} catch (NumberFormatException ex) {
			scrVal = 0;
			screen.setText("Invalid Input");
		} catch (ArithmeticException ex) {
			scrVal = 0;
			screen.setText(ex.getMessage());
		}
	} //buttonPick()
	
	public static void main(String args[]) {
		CalculatorGUI cG = new CalculatorGUI();
		cG.setSize(320, 180);
	} //main()
	
	/**
	Adds the last pressed number to the string on the screen
	@param n	The number to be added
	*/
	private void addnum(char n) {
		int added = Character.getNumericValue(n); //the number value of the character n
		if (!overwrite) { //if value not to be overwritten
			if (scrVal >= 0) //currently a positive value
				scrVal = (10 * scrVal) + added; //adds the extra number
			else //currently a negative value
				scrVal = (10 * scrVal) - added; //subracts the extra number
		} else { //if value to be overwritten
			scrVal = added; //sets current value to new button
			overwrite = false;
		}
		screen.setText(Integer.toString(scrVal)); //puts up the new value
	} //addnum()
	
	/**
	Enters a new operator
	@param op	The operator pressed
	*/
	private void newOp(char op) {
		if (lastOp == '=') //if this does not follow other operations
			lastTerm = scrVal; //saves screen value
		else //if this does follow other operations
			lastTerm = Calculator.calc(lastTerm, scrVal, lastOp); //calculates the last expression into the first operator term
		lastOp = op; //saves the current operator
		overwrite = true; //puts the screen in overwrite mode
	} //newOp()
	
	/**
	Calculates the current expression
	*/
	private void comp() {
		lastTerm = Calculator.calc(lastTerm, scrVal, lastOp); //calculates the last expression into the first operator term
		scrVal = lastTerm; //sets the current value to the result of the computation
		screen.setText(Integer.toString(scrVal)); //displays the result
		lastOp = '='; //saves the current operator as an equals sign - meaning no operator
		overwrite = true; //puts the screen in overwrite mode
	}
} //CalculatorGUI class

