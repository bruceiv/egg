/**
The PhoneServerGUI class is the GUI for the phone number lookup service's server 
@author Aaron Moss
*/
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;

public class PhoneServerGUI extends JFrame
							implements ActionListener, AddableListPane, AddableServerPane {
	private PhoneListManager listMan; //the server functionality of the GUI
	
	private JTextArea listPane; //the text area for display of the phone list
	private JScrollPane listScroll; //scrolling functionality for the phone directory
	private JTextField nameField; //the field to enter a new name
	private JTextField numField; //the field to enter a new phone number
	private JButton addButton; //the button to add a new name and phone number to the directory
	private JButton removeButton; //the button to remove a name from the directory
	private JTextField fileField; //the field to enter a file name
	private JButton saveButton; //the button to save the list to the file
	private JButton loadButton; //the button to load a list from the specified file
	private JTextArea statusPane; //the text area for the connection status
	private JScrollPane statusScroll; //scrolling functionality for the connection status
	private JLabel connectLabel; //the label for displaying the number of connections
	private int numConnections; //the current number of clients connected
	
	public PhoneServerGUI() {
		//initializes pane
		Container pane = getContentPane();
		pane.setLayout(new BorderLayout());
		
		//builds directory view
		JPanel listPanel = new JPanel(); //the panel for the directory view
		listPane = new JTextArea(17, 20);
		listPane.setEditable(false); //removes user ability to edit pane
		listScroll = new JScrollPane(listPane);
		listScroll.setWheelScrollingEnabled(true); //adds scrolling from the mouse wheel
		listScroll.setVerticalScrollBarPolicy(ScrollPaneConstants.VERTICAL_SCROLLBAR_ALWAYS);
			//adds vertical scrollbar to pane
		listPanel.add(listScroll);
		listPanel.setBorder(BorderFactory.createTitledBorder("Phone Directory: "));
		
		//builds directory addition pane
		JPanel addPanel = new JPanel();
		addPanel.setLayout(new BoxLayout(addPanel, BoxLayout.Y_AXIS));
		addPanel.add(new JLabel("Name:"));
		nameField = new JTextField();
		nameField.addActionListener(this);
		addPanel.add(nameField);
		addPanel.add(new JLabel("Number:"));
		numField = new JTextField();
		numField.addActionListener(this);
		addPanel.add(numField);
		
		JPanel addRemovePanel = new JPanel();
		addRemovePanel.setLayout(new BoxLayout(addRemovePanel, BoxLayout.X_AXIS));
		addButton = new JButton("Add");
		addButton.addActionListener(this);
		addRemovePanel.add(addButton);
		removeButton = new JButton("Remove");
		removeButton.addActionListener(this);
		addRemovePanel.add(removeButton);
		addRemovePanel.setAlignmentX((float)0);
		addPanel.add(addRemovePanel);
		addPanel.setBorder(BorderFactory.createTitledBorder("Add to Directory: "));
		
		//builds file management pane
		JPanel filePanel = new JPanel();
		filePanel.setLayout(new BoxLayout(filePanel, BoxLayout.Y_AXIS));
		fileField = new JTextField();
		saveButton = new JButton("Save");
		saveButton.addActionListener(this);
		loadButton = new JButton("Load");
		loadButton.addActionListener(this);
		filePanel.add(fileField);
		JPanel saveLoadPanel = new JPanel();
		saveLoadPanel.setLayout(new BoxLayout(saveLoadPanel, BoxLayout.X_AXIS));
		saveLoadPanel.add(saveButton);
		saveLoadPanel.add(loadButton);
		saveLoadPanel.setAlignmentX((float)0);
		filePanel.add(saveLoadPanel);
		filePanel.setBorder(BorderFactory.createTitledBorder("Save/Load File: "));
		
		//builds list management pane
		JPanel userPanel = new JPanel();
		userPanel.setLayout(new BorderLayout());
		userPanel.add("North", addPanel);
		userPanel.add("South", filePanel);
		
		//builds server status viewer
		JPanel serverPanel = new JPanel(); //the panel for the area
		serverPanel.setLayout(new BorderLayout());
		statusPane = new JTextArea(17, 25);
		statusPane.setEditable(false); //removes user ability to edit pane
		statusScroll = new JScrollPane(statusPane); //adds scrolling to connection status pane
		statusScroll.setWheelScrollingEnabled(true); //adds scrolling from the mouse wheel
		statusScroll.setVerticalScrollBarPolicy(ScrollPaneConstants.VERTICAL_SCROLLBAR_ALWAYS);
			//adds vertical scrollbar to pane
		serverPanel.add("North", statusScroll);
		connectLabel = new JLabel("Connections: 0");
		numConnections = 0; //initializes to zero connections
		serverPanel.add("South", connectLabel);
		serverPanel.setBorder(BorderFactory.createTitledBorder("Server Status: "));
		
		//adds components to window
		pane.add("West", listPanel);
		pane.add("Center", userPanel);
		pane.add("East", serverPanel);
		
		//final operations
		this.setDefaultCloseOperation(EXIT_ON_CLOSE);
		setTitle("Phone Lookup Server");
		pack();
		setVisible(true);
		
		listMan = new PhoneListManager(this); //initializes the server
	} //constructor
	
	public void actionPerformed(ActionEvent e) {
		//is the action related to directory addition
		boolean isDirectoryOp = (e.getSource().equals(nameField)
								 || e.getSource().equals(numField)
								 || e.getSource().equals(addButton));
		if (isDirectoryOp) { //if this is a directory addition
			String newName = nameField.getText(); //the name to be added
			String newNum = stripString(numField.getText()); //the number to be added
			
			//is one of the fields blank?
			boolean fieldIsBlank = (newName.equals("") || newNum.equals(""));
			if (!fieldIsBlank)
				listMan.addToDirectory(newName, newNum);
			else //if there is a blank entry field
				showDialog("Please enter both a name and a number.");
		}
		else if (e.getSource().equals(removeButton)) { //if this is a directory removal
			String remName = nameField.getText();
			if (!remName.equals(""))
				listMan.removeFromDirectory(remName);
			else
				showDialog("Please enter the name of the person to remove.");
		}
		else if (e.getSource().equals(saveButton)) { //if this is a save operation
			String filename = fileField.getText();
			if (!filename.equals("")) //if there is a string in the filename field
				listMan.saveToFile(filename);
			else
				showDialog("Please enter a file name.");
		}
		else if (e.getSource().equals(loadButton)) { //if this is a load operation
			String filename = fileField.getText();
			if (!filename.equals("")) //if there is a string in the filename field
				listMan.loadFromFile(filename);
			else
				showDialog("Please enter a file name.");
		}
	} //actionPerformed()
	
	/**
	Updates the label for number of connections
	Included to implement AddableServerPane
	@param changeConnections	The number of connections added or dropped (represented by a negative number)
	*/
	public void incrementConnectionLabel() {
		numConnections++; //updates number of connections
		connectLabel.setText("Connections: " + numConnections);
	} //updateConnectionLabel()
	
	/**
	Included to implement AddableServerPane
	@param s	the string to be appended to the server status pane, along with a line return
	*/
	public void addToServerDisplay(String s) {
		statusPane.append(s + "\n");
	} //addToServerDisplay()
	
	/**
	Included to implement AddableListPane
	@param s	the string to be appended to the server status pane, along with a line return
	*/
	public void addToListDisplay(String s) {
		listPane.append(s);
	} //addToListDisplay()
	
	/**
	Included to implement AddableListPane
	*/
	public void blankListDisplay() {
		listPane.setText("");
	} //blankListDisplay()
	
	/**
	Displays a message dialog
	Included for implentation of AddableServerPane
	@param message	The message in the dialog
	*/
	public void showDialog(String message) {
		JOptionPane.showMessageDialog(this, message);
	} //showDialog()
	
	public static void main(String[] args) {
		new PhoneServerGUI();
	} //main()
	
	/**
	@param s		The string to be stripped
	@return 		The string with all but numeric digits stripped
	*/
	private String stripString(String s) {
		String newString = ""; //the new, modified string
		for (int i = 0; i < s.length(); i++) {
			char newChar = s.charAt(i);
			switch (newChar) {
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
					newString = newString + newChar; //appends character to string
					break;
			} //switch
		} //for
		return newString;
	} //stripString()
} //PhoneServerGUI

