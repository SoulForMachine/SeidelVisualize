import sys

def main(argv):
    filename = argv[1]
    with open(filename) as f:
        content = f.readlines()

    # Delete the header.
    content.pop(0)

    # Take only fist two values.
    for i in range(len(content)):
        content[i] = ",".join(content[i].split(",")[0:2])

    # Remove duplicated last vertex of each polygon.
    firstLine = ""
    for i in range(len(content)):
        if firstLine == "":
            firstLine = content[i]
        else:
            if firstLine == content[i]:
                content[i] = "*\n"
                firstLine = ""

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
            coords = line.split(",")
            pointArr.append(Point(float(coords[0]) * scale, float(coords[1]) * scale))

    # Find the middle point.
    midPt = Point(0.0, 0.0)
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
    if len(sys.argv) == 3:
        main(sys.argv)
    else:
        print("Usage: python gis-csv-to-poly.py <input file> <scale>")
