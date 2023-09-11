import sys

def main(argv):
    filename = argv[1]
    with open(filename) as f:
        content = f.readlines()

    class Point:
        def __init__(self, x, y) -> None:
            self.x = x
            self.y = y

    pointArr = []
    scale = float(argv[2])

    # Create an array of scaled points.
    for line in content:
        if line == "*\n":
            pointArr.append(None)
        else:
            coords = line.split(" ")
            pointArr.append(Point(float(coords[0]) * scale, float(coords[1]) * scale))

    # Find the middle point.
    midPt = Point(0.0, 0.0)
    if len(sys.argv) == 4 and argv[3] == "-c":
        numPts = len(pointArr)
        for pt in pointArr:
            if pt is not None:
                midPt.x += pt.x / numPts
                midPt.y += pt.y / numPts

    # Write the points to a file.
    with open("out.poly", "w") as f:
        for pt in pointArr:
            if pt is not None:
                newPt = Point(pt.x - midPt.x, pt.y - midPt.y)
                f.write("{:.6f} {:.6f}\n".format(newPt.x, newPt.y))
            else:
                f.write("*\n")

if __name__ == "__main__":
    if len(sys.argv) == 3 or len(sys.argv) == 4:
        main(sys.argv)
    else:
        print("Usage: python scale-and-center-poly.py <input file> <scale> [-c]")
