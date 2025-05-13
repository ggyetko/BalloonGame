
import os
import curses
import sys

RED = '\033[31m'
RESET = '\033[0m'

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

    for x in range(leftEdge, rightEdge+1):
        stalactite = mountainHeight[(terrain[x] & 0b00111000) >> 3]
        stalacmite = mountainHeight[(terrain[x] & 0b00000111)]
        for y in range(0,stalactite):
            sys.stdout.write("\033[{};{}H".format(y+2, x-leftEdge+1))
            sys.stdout.write("\033[K")
            if (x==editor):
                sys.stdout.write(f"{RED}X")
            else:
                sys.stdout.write(f"{RESET}X")
            sys.stdout.flush()
        for y in range(25,25-stalacmite,-1):
            sys.stdout.write("\033[{};{}H".format(y+2, x-leftEdge+1))
            sys.stdout.write("\033[K")
            if (x==editor):
                sys.stdout.write(f"{RED}X")
            else:
                sys.stdout.write(f"{RESET}X")
            sys.stdout.flush()
        if terrain[x] & 0b11000000:
            cityNumber = (terrain[x] & 0b11000000) >> 6
            text = str(cityNumber)*3
            for cityy in range(25-stalacmite-2,25-stalacmite+1):
                sys.stdout.write("\033[{};{}H".format(cityy+2, x-leftEdge+1))
                sys.stdout.write("\033[K")
                sys.stdout.write(text)
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
            if x-offset >= 0 and getH(x-offset) < newHeight:
                left = True
                setH(x-offset, newHeight)
        if right:
            right = False
            if x+offset < 256 and getH(x+offset) < newHeight:
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
            if x-offset >= 0 and getD(x-offset) < newDrop:
                left = True
                setD(x-offset, newDrop)
        if right:
            right = False
            if x+offset < 256 and getD(x+offset) < newDrop:
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

def dumpTerrain(filename, terNum):
    allLines = []
    file = open(filename, "r")
    for line in file:
        allLines.append(line)
    file.close()

    terIndex = 0
    ignore = False
    file = open(filename, "w")
    for line in allLines:
        if ignore:
            if "}" in line:
                ignore = False
        else:
            file.write(line)
            if "{" in line:
                terIndex +=1
                if terIndex == terNum:
                    ignore = True
                    for line in range (0,16):
                        outText = ""
                        cityNum = 0
                        for value in range (0,16):
                            # get value
                            conversion = oct(terrain[line*16+value])
                            if terrain[line*16+value] >> 6:
                                cityNum = terrain[line*16+value] >> 6
                            # convert to C-style octal
                            conversion = conversion[0] + conversion[2:]
                            outText += conversion + ", "
                        if cityNum:
                            file.write(outText + "// City " + str(cityNum) + "\n")
                        else:
                            file.write(outText + "\n")
                    file.write("},\n")
            
            
def readTerrain(filename, terNum):
    file = open(filename, "r")
    terIndex = 0
    for line in file:
        if "{" in line:
            terIndex += 1
        if terIndex == terNum:
            break

    index = 0
    for line in file:
        if "}" in line:
            break
        splitter = line.split(",")
        for value in splitter:
            stripped = value.strip()
            if len(stripped) and stripped[0] == "0":
                terrain[index] = int(value,8)
                index += 1

def countTerrains(filename):
    file = open(filename, "r")
    index = 0
    for line in file:
        if "{" in line:
            index += 1
    return index

def moveCity(editPoint, cityNumber):
    # look for the city first, remove it if you find iter
    for x in range(0,256):
        cityTemp = (terrain[x] & 0b11000000) >> 6
        if cityTemp == cityNumber:
            terrain[x] &= 0b00111111
            break
    # now put it in the right spot
    terrain[editPoint] = (terrain[editPoint] & 0b00111111) | (cityNumber << 6)


terCount = countTerrains("terrainFile.c")

terNum = 0
while terNum == 0 or terNum > terCount:
    print ("Select Terrain 1 - {}:".format(terCount))
    choice = input()
    print("Choice",choice)
    terNum = int(choice)
    

readTerrain("terrainFile.c", terNum)

currEnd = 200

currStart = 0
editPoint = 10

stdscr = curses.initscr()
curses.noecho()  # Disable echoing of characters
curses.cbreak()  # React to keys instantly, without waiting for Enter

while 1:
    while editPoint + 10 > currEnd and currEnd < 255:
        currStart += 1
        currEnd += 1
    while currStart + 10 > editPoint and currStart > 0:
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
    
    elif key > 48 and key < 52:
        moveCity(editPoint, key-48)
        
    elif key == 120:
        dumpTerrain("terrainFile.c", terNum)
        break

curses.nocbreak()
curses.echo()
curses.endwin()
