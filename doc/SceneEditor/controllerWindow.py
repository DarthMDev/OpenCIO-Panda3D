#################################################################
# controllerWindow.py
# Written by Yi-Hong Lin, yihhongl@andrew.cmu.edu, 2004
#################################################################
# Import Tkinter, Pmw, and the floater code from this directory tree.
from AppShell import *
from TkGlobal import *
import VectorWidgets
import Floater
import string

# Define the Category
KEYBORAD = 'Keyboard-'
TRACKER = 'Tarcker-'

class controllerWindow(AppShell):
    #################################################################
    # This will open a talk window for user to set the control mechanism
    # In here, user can choose to control what object by keyboard or other inputs.
    #################################################################
    
    # Override class variables
    appname = 'Controller Panel'
    frameWidth  = 500
    frameHeight = 500

    widgetsDict = {} # category-id : widget obj

    # setup the type of controller we handle here.
    controllerList = ['Keyboard',
                      'Tracker']
    
    # Default Keyboard setting
    keyboardMapDict = {}
    keyboardSpeedDict = {}
    
    def __init__(self, listOfObj, controlType , dataList, parent = None, **kw):
        if controlType == 'Keyboard':
            self.nodePath = dataList[0] # Default setting -> mainly used for Keyboard control now.
            self.nameOfNode = self.nodePath.getName()
            self.controllType = 'Keyboard'
            self.keyboardMapDict.clear()
            self.keyboardMapDict = dataList[1]
            self.keyboardSpeedDict.clear()
            self.keyboardSpeedDict = dataList[2]
            
        self.listOfObj = listOfObj
        self.keepControl = False
        
        INITOPT = Pmw.INITOPT
        optiondefs = (
            ('title',               self.appname,       None),
            )
        self.defineoptions(kw, optiondefs)

        # Initialize the superclass
        AppShell.__init__(self)

        # Execute option callbacks
        self.initialiseoptions(controllerWindow)

        self.parent.resizable(False,False) ## Disable the ability to resize for this Window.

    def createInterface(self):
        # Handle to the toplevels interior
        interior = self.interior()
        menuBar = self.menuBar
        
        # We don't need menu bar here
        self.menuBar.destroy() 

        # Create a frame to hold all stuff
        mainFrame = Frame(interior)

        # A comboBox to switch the controller
        frame = Frame(mainFrame)
        self.cotrollerTypeEntry = self.createcomponent(
            'Controller Type', (), None,
            Pmw.ComboBox, (frame,),
            labelpos = W, label_text='Controller Type:', entry_width = 20,entry_state = DISABLED,
            selectioncommand = self.setControllerType,
            scrolledlist_items = self.controllerList)
        self.cotrollerTypeEntry.pack(side=LEFT)
        frame.pack(side=TOP, fill=X, expand=False, pady = 3)
        self.cotrollerTypeEntry.selectitem('Keyboard', setentry=True)
        
        self.inputZone = Pmw.Group(mainFrame, tag_pyclass = None)
        self.inputZone.pack(fill='both',expand=1)
        settingFrame = self.inputZone.interior()

        ###################################################
        # Notebook pages for specific controller setting  #
        ###################################################
        self.contentWidge = self.createcomponent(
            'scrolledFrame',
            (), None,
            Pmw.ScrolledFrame, (settingFrame,),
            hull_width = 200, hull_height = 300,
            usehullsize = 1)
        self.contentFrame = self.contentWidge.component('frame')
        self.contentWidge.pack(fill = 'both', expand = 1,padx = 3, pady = 5)
        self.objNotebook = Pmw.NoteBook(self.contentFrame, tabpos = None,
                                        borderwidth = 0)
        keyboardPage = self.objNotebook.add('Keyboard')
        tarckerPage = self.objNotebook.add('Tracker')
        self.objNotebook.selectpage('Keyboard')
        self.objNotebook.pack(side = TOP, fill='both',expand=False)
        # Put this here so it isn't called right away
        self.objNotebook['raisecommand'] = self.updateControlInfo

        # Keyboard  setting
        assignFrame = Frame(keyboardPage)

        Interior = Frame(assignFrame)
        widget = self.createcomponent(
            'Target Type', (), None,
            Pmw.ComboBox, (Interior,),
            labelpos = W, label_text='Target Object:', entry_width = 20, entry_state = DISABLED,
            selectioncommand = self.setTargetObj,
            scrolledlist_items = self.listOfObj)
        widget.pack(side=LEFT, padx=3)
        Interior.pack(side=TOP, fill=X, expand=True, pady = 5)
        widget.selectitem(self.nameOfNode, setentry=True)
        self.widgetsDict[KEYBORAD+'ObjList'] = widget

        inputZone = Pmw.Group(assignFrame, tag_pyclass = None)
        inputZone.pack(fill='both',expand=1)
        settingFrame = inputZone.interior()
        
        Interior = Frame(settingFrame)
        widget = Label(Interior, text = 'Assign a Key For:').pack(side=LEFT, expand = False)
        Interior.pack(side=TOP, fill=X, expand=True,pady = 6 )
        
        Interior = Frame(settingFrame)
        widget = Label(Interior, text = 'Forward   :', width = 20, anchor = W).pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Forward key', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardMapDict['KeyForward'],
            labelpos = W, label_text='Key :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'KeyForward'] = widget
        widget = Label(Interior, text = '   ').pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Forward Speed', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardSpeedDict['SpeedForward'],
            labelpos = W, label_text='Speed :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'SpeedForward'] = widget
        widget = Label(Interior, text = 'Per Seceond').pack(side=LEFT, expand = False)
        Interior.pack(side=TOP, fill=X, expand=True, pady = 4 )

        Interior = Frame(settingFrame)
        widget = Label(Interior, text = 'Backward  :', width = 20, anchor = W).pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Backward key', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardMapDict['KeyBackward'],
            labelpos = W, label_text='Key :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'KeyBackward'] = widget
        widget = Label(Interior, text = '   ').pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Backward Speed', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardSpeedDict['SpeedBackward'],
            labelpos = W, label_text='Speed :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'SpeedBackward'] = widget
        widget = Label(Interior, text = 'Per Seceond').pack(side=LEFT, expand = False)
        Interior.pack(side=TOP, fill=X, expand=True, pady = 4 )

        Interior = Frame(settingFrame)
        widget = Label(Interior, text = 'Right     :', width = 20, anchor = W).pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Right key', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardMapDict['KeyRight'],
            labelpos = W, label_text='Key :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'KeyRight'] = widget
        widget = Label(Interior, text = '   ').pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Right Speed', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardSpeedDict['SpeedRight'],
            labelpos = W, label_text='Speed :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'SpeedRight'] = widget
        widget = Label(Interior, text = 'Per Seceond').pack(side=LEFT, expand = False)
        Interior.pack(side=TOP, fill=X, expand=True, pady = 4 )

        Interior = Frame(settingFrame)
        widget = Label(Interior, text = 'Left      :', width = 20, anchor = W).pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Left key', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardMapDict['KeyLeft'],
            labelpos = W, label_text='Key :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'KeyLeft'] = widget
        widget = Label(Interior, text = '   ').pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Left Speed', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardSpeedDict['SpeedLeft'],
            labelpos = W, label_text='Speed :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'SpeedLeft'] = widget
        widget = Label(Interior, text = 'Per Seceond').pack(side=LEFT, expand = False)
        Interior.pack(side=TOP, fill=X, expand=True, pady = 4 )

        Interior = Frame(settingFrame)
        widget = Label(Interior, text = 'Up        :', width = 20, anchor = W).pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Up key', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardMapDict['KeyUp'],
            labelpos = W, label_text='Key :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'KeyUp'] = widget
        widget = Label(Interior, text = '   ').pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Up Speed', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardSpeedDict['SpeedUp'],
            labelpos = W, label_text='Speed :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'SpeedUp'] = widget
        widget = Label(Interior, text = 'Per Seceond').pack(side=LEFT, expand = False)
        Interior.pack(side=TOP, fill=X, expand=True, pady = 4 )

        Interior = Frame(settingFrame)
        widget = Label(Interior, text = 'Down      :', width = 20, anchor = W).pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Down key', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardMapDict['KeyDown'],
            labelpos = W, label_text='Key :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'KeyDown'] = widget
        widget = Label(Interior, text = '   ').pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Down Speed', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardSpeedDict['SpeedDown'],
            labelpos = W, label_text='Speed :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'SpeedDown'] = widget
        widget = Label(Interior, text = 'Per Seceond').pack(side=LEFT, expand = False)
        Interior.pack(side=TOP, fill=X, expand=True, pady = 4 )

        Interior = Frame(settingFrame)
        widget = Label(Interior, text = 'Turn Right:', width = 20, anchor = W).pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Turn Right key', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardMapDict['KeyTurnRight'],
            labelpos = W, label_text='Key :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'KeyTurnRight'] = widget
        widget = Label(Interior, text = '   ').pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Turn Right Speed', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardSpeedDict['SpeedTurnRight'],
            labelpos = W, label_text='Speed :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'SpeedTurnRight'] = widget
        widget = Label(Interior, text = 'Per Seceond').pack(side=LEFT, expand = False)
        Interior.pack(side=TOP, fill=X, expand=True, pady = 4 )

        Interior = Frame(settingFrame)
        widget = Label(Interior, text = 'Turn Left :', width = 20, anchor = W).pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Turn Left key', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardMapDict['KeyTurnLeft'],
            labelpos = W, label_text='Key :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'KeyTurnLeft'] = widget
        widget = Label(Interior, text = '   ').pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Turn Left Speed', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardSpeedDict['SpeedTurnLeft'],
            labelpos = W, label_text='Speed :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'SpeedTurnLeft'] = widget
        widget = Label(Interior, text = 'Per Seceond').pack(side=LEFT, expand = False)
        Interior.pack(side=TOP, fill=X, expand=True, pady = 4 )

        Interior = Frame(settingFrame)
        widget = Label(Interior, text = 'Turn UP   :', width = 20, anchor = W).pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Turn UP key', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardMapDict['KeyTurnUp'],
            labelpos = W, label_text='Key :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'KeyTurnUp'] = widget
        widget = Label(Interior, text = '   ').pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Turn UP Speed', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardSpeedDict['SpeedTurnUp'],
            labelpos = W, label_text='Speed :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'SpeedTurnUp'] = widget
        widget = Label(Interior, text = 'Per Seceond').pack(side=LEFT, expand = False)
        Interior.pack(side=TOP, fill=X, expand=True, pady = 4 )

        Interior = Frame(settingFrame)
        widget = Label(Interior, text = 'Turn Down :', width = 20, anchor = W).pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Turn Down key', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardMapDict['KeyTurnDown'],
            labelpos = W, label_text='Key :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'KeyTurnDown'] = widget
        widget = Label(Interior, text = '   ').pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Turn Down Speed', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardSpeedDict['SpeedTurnDown'],
            labelpos = W, label_text='Speed :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'SpeedTurnDown'] = widget
        widget = Label(Interior, text = 'Per Seceond').pack(side=LEFT, expand = False)
        Interior.pack(side=TOP, fill=X, expand=True, pady = 4 )

        Interior = Frame(settingFrame)
        widget = Label(Interior, text = 'Roll Right:', width = 20, anchor = W).pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Roll Right key', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardMapDict['KeyRollRight'],
            labelpos = W, label_text='Key :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'KeyRollRight'] = widget
        widget = Label(Interior, text = '   ').pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Roll Right Speed', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardSpeedDict['SpeedRollRight'],
            labelpos = W, label_text='Speed :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'SpeedRollRight'] = widget
        widget = Label(Interior, text = 'Per Seceond').pack(side=LEFT, expand = False)
        Interior.pack(side=TOP, fill=X, expand=True, pady = 4 )

        Interior = Frame(settingFrame)
        widget = Label(Interior, text = 'Roll Left :', width = 20, anchor = W).pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Roll Left key', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardMapDict['KeyRollLeft'],
            labelpos = W, label_text='Key :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'KeyRollLeft'] = widget
        widget = Label(Interior, text = '   ').pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Roll Left Speed', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardSpeedDict['SpeedRollLeft'],
            labelpos = W, label_text='Speed :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'SpeedRollLeft'] = widget
        widget = Label(Interior, text = 'Per Seceond').pack(side=LEFT, expand = False)
        Interior.pack(side=TOP, fill=X, expand=True, pady = 4 )

        Interior = Frame(settingFrame)
        widget = Label(Interior, text = 'Scale UP :', width = 20, anchor = W).pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Scale UP key', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardMapDict['KeyScaleUp'],
            labelpos = W, label_text='Key :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'KeyScaleUp'] = widget
        widget = Label(Interior, text = '   ').pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Scale UP Speed', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardSpeedDict['SpeedScaleUp'],
            labelpos = W, label_text='Speed :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'SpeedScaleUp'] = widget
        widget = Label(Interior, text = 'Per Seceond').pack(side=LEFT, expand = False)
        Interior.pack(side=TOP, fill=X, expand=True, pady = 4 )

        Interior = Frame(settingFrame)
        widget = Label(Interior, text = 'Scale Down:', width = 20, anchor = W).pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Scale Down key', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardMapDict['KeyScaleDown'],
            labelpos = W, label_text='Key :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'KeyScaleDown'] = widget
        widget = Label(Interior, text = '   ').pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Scale Down Speed', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardSpeedDict['SpeedScaleDown'],
            labelpos = W, label_text='Speed :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'SpeedScaleDown'] = widget
        widget = Label(Interior, text = 'Per Seceond').pack(side=LEFT, expand = False)
        Interior.pack(side=TOP, fill=X, expand=True, pady = 4 )

        Interior = Frame(settingFrame)
        widget = Label(Interior, text = 'Scale X UP :', width = 20, anchor = W).pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Scale X UP key', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardMapDict['KeyScaleXUp'],
            labelpos = W, label_text='Key :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'KeyScaleXUp'] = widget
        widget = Label(Interior, text = '   ').pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Scale X UP Speed', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardSpeedDict['SpeedScaleXUp'],
            labelpos = W, label_text='Speed :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'SpeedScaleXUp'] = widget
        widget = Label(Interior, text = 'Per Seceond').pack(side=LEFT, expand = False)
        Interior.pack(side=TOP, fill=X, expand=True, pady = 4 )

        Interior = Frame(settingFrame)
        widget = Label(Interior, text = 'Scale X Down:', width = 20, anchor = W).pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Scale X Down key', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardMapDict['KeyScaleXDown'],
            labelpos = W, label_text='Key :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'KeyScaleXDown'] = widget
        widget = Label(Interior, text = '   ').pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Scale Down X Speed', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardSpeedDict['SpeedScaleXDown'],
            labelpos = W, label_text='Speed :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'SpeedScaleXDown'] = widget
        widget = Label(Interior, text = 'Per Seceond').pack(side=LEFT, expand = False)
        Interior.pack(side=TOP, fill=X, expand=True, pady = 4 )

        Interior = Frame(settingFrame)
        widget = Label(Interior, text = 'Scale Y UP :', width = 20, anchor = W).pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Scale Y UP key', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardMapDict['KeyScaleYUp'],
            labelpos = W, label_text='Key :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'KeyScaleYUp'] = widget
        widget = Label(Interior, text = '   ').pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Scale Y UP Speed', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardSpeedDict['SpeedScaleYUp'],
            labelpos = W, label_text='Speed :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'SpeedScaleYUp'] = widget
        widget = Label(Interior, text = 'Per Seceond').pack(side=LEFT, expand = False)
        Interior.pack(side=TOP, fill=X, expand=True, pady = 4 )

        Interior = Frame(settingFrame)
        widget = Label(Interior, text = 'Scale Y Down:', width = 20, anchor = W).pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Scale Y Down key', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardMapDict['KeyScaleYDown'],
            labelpos = W, label_text='Key :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'KeyScaleYDown'] = widget
        widget = Label(Interior, text = '   ').pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Scale Down XY Speed', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardSpeedDict['SpeedScaleYDown'],
            labelpos = W, label_text='Speed :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'SpeedScaleYDown'] = widget
        widget = Label(Interior, text = 'Per Seceond').pack(side=LEFT, expand = False)
        Interior.pack(side=TOP, fill=X, expand=True, pady = 4 )

        Interior = Frame(settingFrame)
        widget = Label(Interior, text = 'Scale Z UP :', width = 20, anchor = W).pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Scale Z UP key', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardMapDict['KeyScaleZUp'],
            labelpos = W, label_text='Key :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'KeyScaleZUp'] = widget
        widget = Label(Interior, text = '   ').pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Scale Z UP Speed', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardSpeedDict['SpeedScaleZUp'],
            labelpos = W, label_text='Speed :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'SpeedScaleZUp'] = widget
        widget = Label(Interior, text = 'Per Seceond').pack(side=LEFT, expand = False)
        Interior.pack(side=TOP, fill=X, expand=True, pady = 4 )

        Interior = Frame(settingFrame)
        widget = Label(Interior, text = 'Scale Z Down:', width = 20, anchor = W).pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Scale Z Down key', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardMapDict['KeyScaleZDown'],
            labelpos = W, label_text='Key :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'KeyScaleZDown'] = widget
        widget = Label(Interior, text = '   ').pack(side=LEFT, expand = False)
        widget = self.createcomponent(
            'Scale Down Z Speed', (), None,
            Pmw.EntryField, (Interior,),
            value = self.keyboardSpeedDict['SpeedScaleZDown'],
            labelpos = W, label_text='Speed :', entry_width = 10)
        widget.pack(side=LEFT, expand = False)
        self.widgetsDict[KEYBORAD+'SpeedScaleZDown'] = widget
        widget = Label(Interior, text = 'Per Seceond').pack(side=LEFT, expand = False)
        Interior.pack(side=TOP, fill=X, expand=True, pady = 4 )

        assignFrame.pack(side=TOP, expand=True, fill = X)
        keyboardPage.pack(side=TOP, expand=True, fill = X)
        
        ####################################################################
        ####################################################################
        # End of Keyboard control page
        ####################################################################
        ####################################################################
        # Pack the mainFrame
        frame = Frame(mainFrame)
        widget = Button(frame, text='OK', width = 13, command=self.ok_press).pack(side=RIGHT)
        widget = Button(frame, text='Enable Control', width = 13, command=self.enableControl).pack(side=LEFT)
        widget = Button(frame, text='Disable Control', width = 13, command=self.disableControl).pack(side=LEFT)
        widget = Button(frame, text='Save & Keep', width = 13, command=self.saveKeepControl).pack(side=LEFT)
        frame.pack(side = BOTTOM, expand=1, fill = X)
        mainFrame.pack(expand=1, fill = BOTH)

    def onDestroy(self, event):
        # Check if user wish to keep the control after the window closed.
        if not self.keepControl:
            messenger.send('ControlW_controlDisable',[self.controllType])
        messenger.send('ControlW_close')
        '''
        If you have open any thing, please rewrite here!
        '''
        pass

    def ok_press(self):
        ####################################################################
        # ok_press(self)
        # After user clicked on "OK" button, this function will be called.
        # This function will collect data from the panel and send it back to
        # sceneEditor and close the window. It won't activate control at all.
        ####################################################################
        if self.controllType=='Keyboard':
            for index in self.keyboardMapDict:
                self.keyboardMapDict[index] = self.widgetsDict['Keyboard-'+index].getvalue()
            for index in self.keyboardSpeedDict:
                self.keyboardSpeedDict[index] = float(self.widgetsDict['Keyboard-'+index].getvalue())
            messenger.send('ControlW_controlSetting', ['Keyboard', [self.nodePath, self.keyboardMapDict, self.keyboardSpeedDict]])
        self.quit()
        return

    def enableControl(self):
        ####################################################################
        # enableControl(self)
        # Call back function.
        # THis function will be called each time when user clicks on the
        # "Enable Control" button. This function will do pretty much
        # the same thing with ok_press function, except that this function
        # will activate the control process in sceneEditor.
        # However, if user didn't click on the button "Keep ANd Save,"
        # the control process will be terminated when user closed the panel.
        ####################################################################
        if self.controllType=='Keyboard':
            for index in self.keyboardMapDict:
                self.keyboardMapDict[index] = self.widgetsDict['Keyboard-'+index].getvalue()
            for index in self.keyboardSpeedDict:
                self.keyboardSpeedDict[index] = float(self.widgetsDict['Keyboard-'+index].getvalue())
            messenger.send('ControlW_controlEnable', ['Keyboard', [self.nodePath, self.keyboardMapDict, self.keyboardSpeedDict]])
        return

    def disableControl(self):
        ####################################################################
        # disableControl(self)
        # This function will send out a message to sceneEditor to stop the
        # control task.
        ####################################################################
        messenger.send('ControlW_controlDisable',[self.controllType])
        return

    def setControllerType(self, typeName = 'Keyboard'):
        #################################################################
        # setControllerType(self, typeName = 'Keyboard')
        # Call back function
        # This function will be called when user select the type of
        # controller they want on the combo box on the panel.
        # Basically, this function's job is to switch the notebook page to right one.
        #################################################################
        self.controllType = typeName
        if self.controllType=='Keyboard':
            self.objNotebook.selectpage('Keyboard')
        elif self.controllType=='Tracker':
            self.objNotebook.selectpage('Tracker')
            
        return

    def updateControlInfo(self, page=None):
        #################################################################
        # Nothing. Unlike in the lighting panel, we don't have to keep data
        # once user switch the page.
        #################################################################
        return

    def setTargetObj(self, name = None, tracker = None):
        #################################################################
        # setTargetObj(self, name = None)
        # setup the target object we want to control
        #################################################################
        if tracker == None: # Call from Keyboard page.
            if name=='camera':
                self.nodePath = camera
            else:
                messenger.send('ControlW_require',[name])
        return

    def resetNameList(self, list, name = None, nodePath = None):
        ####################################################################
        # resetNameList(self, list, name = None, nodePath = None)
        # This function will be called be sceneEditor in order to reset the
        # object list inside the panel.
        # list here is a name list for all objects that can be set on control
        # in the scene. If user has specify a name and a nodePath in, it will
        # check if the name is equal to the name of current control target.
        # If so, it will change the name showed on panel.
        ####################################################################
        self.widgetsDict['Keyboard-ObjList'].setlist(list)
        if name != None:
            if self.nameOfNode == name:
                self.nameOfNode = self.nodePath.getName()
                self.widgetsDict['Keyboard-ObjList'].selectitem(self.nameOfNode, setentry=True)
        return

    def setNodePathIn(self, nodePath):
        ####################################################################
        # setNodePathIn(self, nodePath)
        # THis function will be called by sceneEditor.
        # After we send out a message to sceneEditor in setTargetObj function,
        # This function will be called by sceneEditor after we get the reference
        # of target object from dataHolder.
        # This function will keep the reference.
        ####################################################################
        self.nodePath = nodePath
        self.nameOfNode = self.nodePath.getName()
        return

    def saveKeepControl(self):
        #################################################################
        # saveKeepControl(self)
        # When any time user has click on the "Save & Keep" button,
        # This function will be called.
        # This function will send out the message to sceneEditor to process
        # the saving. Also, this function will change the "self.keepControl" flag.
        # So, when user closes the window with control enabled, it will keep
        # the control process runnning. otherwise program will disable the
        # control automatically when panel has been closed.
        #
        # It doesn't mean that this function will automatically enable the
        # control when user closed the window!!
        # This flag will only decide that we will send out a "stopControl"
        # message or not.
        #
        #################################################################
        self.keepControl = True
        if self.controllType=='Keyboard':
            for index in self.keyboardMapDict:
                self.keyboardMapDict[index] = self.widgetsDict['Keyboard-'+index].getvalue()
            for index in self.keyboardSpeedDict:
                self.keyboardSpeedDict[index] = float(self.widgetsDict['Keyboard-'+index].getvalue())
            print self.nodePath
            messenger.send('ControlW_saveSetting', ['Keyboard', [self.nodePath, self.keyboardMapDict, self.keyboardSpeedDict]])
        return

    
