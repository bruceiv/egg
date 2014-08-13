/**
The Battleship applet is a basic Battleship-esque game
@author Aaron Moss
*/

import java.applet.*;
import java.awt.*;
import java.awt.event.*;

public class Battleship extends Applet
								implements ActionListener
{
	private int rctX; //The rectangle's top left x-value
	private int rctY; //The rectangle's top left y-value
	private int rctW; //The rectangle's width
	private int rctH; //The rectangle's height
	private int wLim; //The largest width the rectangle can have with a given x-value
	private int hLim; //The largest height the rectangle can have with a given y-value
	
	private Rectangle ship; //the "ship", a rectangle
	
	private TextField xTxt; //the x-value text field
	private TextField yTxt; //the y-value text field
	private Button fireBtn; //the "Fire!" button
	private Button radarBtn; //he "Use Radar" button
	private Label xLbl; //the "x-value:" label
	private Label yLbl; //the "y-value:" label
	private Label shotLbl; //the result label for a shot
	private Label positionLbl; //the "Ship position: ..." label
	
	private final int MAXX = 10; //the largest x-value on the grid
	private final int MAXY = 10; //the larges y-value on the grid
	
	public void init()
	{
		//initialize rctX and rctY to values in the range of the grid
		rctX = (int)((Math.random() * MAXX) + 1.0);
		rctY = (int)((Math.random() * MAXY) + 1.0);
		//set the upper limits for the width and height of the rectangle (must be at least 1)
		wLim = (MAXX - rctX) + 1;
		hLim = (MAXY - rctY) + 1;
		
		//sets the width and height of the rectangle to random values that will fit within the grid
		rctW = (int)((Math.random() * wLim) + 1.0);
		rctH = (int)((Math.random() * hLim) + 1.0);
				
		//sets the dimensions of the "ship" rectangle
		ship = new Rectangle(rctX, rctY, rctW, rctH);
		
		//initializes the interface elements
		xTxt = new TextField();
		yTxt = new TextField();
		fireBtn = new Button("Fire!");
		radarBtn = new Button("Use Radar");
		xLbl = new Label("x-value: ");
		yLbl = new Label("y-value: ");
		shotLbl = new Label("                             ");
		positionLbl = new Label(" Ship Position:                          ");
		
		//adds ActionListeners to the buttons
		fireBtn.addActionListener(this);
		radarBtn.addActionListener(this);
		
		//places the elements on the display
		this.add(xLbl);
		this.add(xTxt);
		this.add(yLbl);
		this.add(yTxt);
		this.add(fireBtn);
		this.add(shotLbl);
		this.add(radarBtn);
		this.add(positionLbl);
	} //end init method
	
	public void actionPerformed(ActionEvent e)
	{
		if(e.getSource() == fireBtn) //the fire button is pressed
		{
			//sets up a couple temporary variables to store the user entered text
			int xFire = Integer.parseInt(xTxt.getText());
			int yFire = Integer.parseInt(yTxt.getText());
			if(ship.contains(xFire, yFire)) //the points in the text boxes are inside the ship
				shotLbl.setText("Hit!");
			else //the points in the text boxes are outside the ship
				shotLbl.setText("Missed!");
		} //end fire button events
		else //the radar button is pressed
		{
			//the -1 in the bottom-right co-ordinates is because the rectangle class needs to have at least a width and a height of 1, but the point the width down, or the height over isn't included
			positionLbl.setText(" Ship Position: (" + rctX + "," + rctY + "), (" + ((rctX + rctW)-1) + "," + ((rctY + rctH)-1) + ")");
		} //end radar button events
	} //end ActionPerformed method
} //end Battleship class
