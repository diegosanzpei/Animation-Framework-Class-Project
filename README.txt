Name: Diego Sanz

Additional Notes:
    Camera:
	- Camera Movement: WASD + EQ 
	- Camera Rotation: Right Click + Mouse Drag || Arrow keys
	- Camera Speed: Use mouse wheel to increase/decrease speed
	- Orbital Camera Rotation: Alt + Right Click + Drag (Rotates around selected node
	- Oribital Camera Movement: Alt + Left Click + Drag (Pans along its local side and up vectors)
	- Orbital Camera Speed is locked
	- Oribital Camera Zoom: Use mouse wheel to move camera forwards backwards
	- Camera Reset: v

    Transform:
	- Press 1 to translate
	- Press 2 to scale
	- press 3 to rotate
	
    Scene:
	- Ctrl + 1: Curve demo
	    
	    - At the bottom-right corner you will see the curve creator
	    - Press the curve that you want to create
	    - By default there is a bezier curve
	    - At the top-left corner you will see the scene graph
	    - Select the default curve in the scene graph to open the inspector
	    - In the inspector, select the curve component to see its content
	
	    Curve Follow Controller:

	        - This controls the curve follower (node that follows the curve)
	        - You can change the speed control (cte speed or distance time function)
	        - You can change the orientation method (frenet or using the y-axis to compute right vector)
	        - The gui is self explanatory.
	
	    Distance Table:

	        - Select this to view the distance table
	        - You can change the table type (uniform or adaptive)
	        - You can rebuild the table with a gui button (do this when you move points in the curve)

	- Ctrl + 2: Animation Demo
            
	    - Loaded with 3 models by default using animation
	    - To Control the animation of each instance, select the node in the scene graph that has the mesh and look in the inspector
	    - In the inspector you can play, pause, loop and change the animation (if the model has various animations)

            Loading Models:

	        - At the top left corner, select file > load > select the file to load
	        - You can also drag and drop any gltf file and it will be loaded
	        - Once loaded, they will appear in the Resources window
		- By clicking on them an instance of the model will be created

	- Ctrl + 3: 1d blending test scene

	    - Go to the top right window (blending editor)
	    - Very straightforward, just change the blend parameter to lerp between animations
	    - You can edit the blend tree: change blend node position and animation
	    - You can add nodes (expand the blend tree and click the add node button)
	    - You can also select a blend node in the ImGui::treeNode and press the delete key to remove the node

	- Ctrl + 4: 1d blending in action

	    - Connect an xbox controller and test the movement
	    - Controls are in a gui window

	- Ctrl + 5: 2d blending test scene

	    - Same as first scene. Go to top right window to change nodes and blend parameter

	- Ctrl + 6: 2d blending in action

	    - Same as second scene. Controls are in a gui window

	- Ctrl + 7: 2 bone Ik demo

		- Click the left mouse button to make the manipulator try to reach the target
		- Target is the mouse position
		- Use the top-right gui window to edit the chain
		- All the required info is in the top right gui window

	- Ctrl + 8: CCD demo

		- Target node selected by default. Move around target with guizmo
		- Use the top-right gui window to edit the chain
		- All the required info is in the top right gui window

	- Ctrl + 9: FABRIK demo

		- Target node selected by default. Move around target with guizmo
		- Use the top-right gui window to edit the chain
		- All the required info is in the top right gui window

	- Ctrl + 0: Skeleton IK demo

		- Target node selected by default. Move around target with guizmo
		- Cessium man's left arm will try to reach target