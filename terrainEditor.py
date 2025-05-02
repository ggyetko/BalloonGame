
import os
import curses
import sys

def clear_screen():
    os.system('cls' if os.name == 'nt' else 'clear')

terrain = [0o011] * 256

mountainHeight = [0,1,2,3,4,6,8,10]

def showTerrain(leftEdge, rightEdge, editor):
    clear_screen()
    sys.stdout.write("\033[{};{}H".format(1, editor-leftEdge+1))
    sys.stdout.write("\033[K")
    sys.stdout.write("v")
    sys.stdout.flush()
    sys.stdout.write("\033[{};{}H".format(28, editor-leftEdge+1))
    sys.stdout.write("\033[K")
    sys.stdout.write("^")
    sys.stdout.flush()

    for x in range(leftEdge, rightEdge):
        stalactite = mountainHeight[(terrain[x] & 0b00111000) >> 3]
        stalacmite = mountainHeight[(terrain[x] & 0b00000111)]
        for y in range(0,stalactite):
            sys.stdout.write("\033[{};{}H".format(y+2, x-leftEdge+1))
            sys.stdout.write("\033[K")
            sys.stdout.write("X")
            sys.stdout.flush()
        for y in range(25,25-stalacmite,-1):
            sys.stdout.write("\033[{};{}H".format(y+2, x-leftEdge+1))
            sys.stdout.write("\033[K")
            sys.stdout.write("X")
            sys.stdout.flush()
    sys.stdout.write("\033[{};{}H".format(29,1))
    sys.stdout.write("\033[K")
    sys.stdout.write(str(editor))
    sys.stdout.flush()

def getH(column):
    return terrain[column] & 0b00000111
def setH(column, height):
    terrain[column] = (terrain[column] & 0b11111000) | height
    
def getD(column):
    return (terrain[column] & 0b00111000) >> 3
def setD(column, drop):
    terrain[column] = (terrain[column] & 0b11000111) | (drop << 3)
#
# SINGLE LIFT
#
def liftStraight(x):
    height = getH(x)
    if height < 7:
        setH(x, height + 1)

def dropStraight(x):
    height = getH(x)
    if height > 0:
        setH(x, height-1)

def reachStraight(x):
    drop = getD(x)
    if drop < 7:
        setD(x, drop+1)

def retractStraight(x):
    drop = getD(x)
    if drop > 0:
        setD(x,drop-1)

#
# MOUNTAIN/VALLEY LIFT
#

def liftMountain(x):
    height = getH(x)
    if height < 7:
        height += 1
        setH(x, height)
    left = True
    right = True 
    offset = 1
    newHeight = height -1
    while left and right:
        if left:
            left = False
            if getH(x-offset) < newHeight:
                left = True
                setH(x-offset, newHeight)
        if right:
            right = False
            if getH(x+offset) < newHeight:
                right = True
                setH(x+offset, newHeight)
        offset += 1
        if offset & 1:
            newHeight -= 1
            
def dropMountain(x):
    drop = getD(x)
    if drop < 7:
        drop += 1
        setD(x, drop)
    left = True
    right = True 
    offset = 1
    newDrop = drop -1
    while left and right:
        if left:
            left = False
            if getD(x-offset) < newDrop:
                left = True
                setD(x-offset, newDrop)
        if right:
            right = False
            if getD(x+offset) < newDrop:
                right = True
                setD(x+offset, newDrop)
        offset += 1
        if offset & 1:
            newDrop -= 1
            
def raiseCliff(x):
    level = getH(x) 
    if level == 7:
        return
    level +=1
    for x1 in range(max(0,x-5), min(255,x+5)):
        if (getH(x1) < level):
            setH(x1,level)

def dropCliff(x):
    level = getD(x) 
    if level == 7:
        return
    level +=1
    for x1 in range(max(0,x-5), min(255,x+5)):
        if getD(x1) < level:
            setD(x1,level)

def dumpTerrain(filename):
    file = open(filename, "w")
    file.write("{\n")
    for line in range (0,16):
        outText = ""
        for value in range (0,16):
            conversion = oct(terrain[line*16+value])
            conversion = conversion[0] + conversion[2:]
            outText += conversion + ", "
        file.write(outText+"\n")
    file.write("}\n")
    
def readTerrain(filename):
    file = open(filename, "r")
    index = 0
    for line in file:
        splitter = line.split(",")
        for value in splitter:
            stripped = value.strip()
            if len(stripped) and stripped[0] == "0":
                terrain[index] = int(value,8)
                index += 1

currEnd = 200

currStart = 0
editPoint = 10

stdscr = curses.initscr()
curses.noecho()  # Disable echoing of characters
curses.cbreak()  # React to keys instantly, without waiting for Enter

readTerrain("myTerrain.txt")

while 1:
    if editPoint + 10 > currEnd and currEnd < 255:
        currStart += 1
        currEnd += 1
    if currStart + 10 > editPoint and currStart > 0:
        currStart -= 1
        currEnd -= 1
        
    showTerrain(currStart, currEnd, editPoint)
    
    key = stdscr.getch()
    if key == 100 and editPoint < 255:
        editPoint += 1
    elif key == 97 and editPoint > 0:
        editPoint -= 1
    elif key == 43 and currEnd < 255:
        currEnd += 1
    
    elif key==101 and editPoint < 245:
        editPoint += 10
    elif key==113 and editPoint > 10:
        editPoint -= 10
        
    elif key == 104:
        liftStraight(editPoint)
    elif key == 110: #n
        dropStraight(editPoint)
    elif key == 54:
        retractStraight(editPoint)
    elif key == 121:
        reachStraight(editPoint)
        
    elif key == 106: #j
        liftMountain(editPoint)
    elif key == 117: #u
        dropMountain(editPoint)
        
    elif key == 107: #k
        raiseCliff(editPoint)
    elif key == 105: #i
        dropCliff(editPoint)
        
    elif key == 120:
        dumpTerrain("myTerrain.txt")
        break

curses.nocbreak()
curses.echo()