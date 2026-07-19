def rotate_points(points, n, angle):
    cx = (n - 1) / 2.0
    cy = (n - 1) / 2.0
    new_points = []
    for x, y in points:
        if angle == 90:
            nx = cx + (y - cy)
            ny = cy - (x - cx)
        elif angle == 180:
            nx = 2 * cx - x
            ny = 2 * cy - y
        elif angle == 270:
            nx = cx - (y - cy)
            ny = cy + (x - cx)
        else:
            raise ValueError("角度必须是90、180或270")
        new_points.append((round(nx), round(ny)))
    return new_points

def format_points(points):
    return ",".join(f"{{{x},{y}}}" for x, y in points)

def main():
    print("请输入网格尺寸（3 或 4）：")
    while True:
        try:
            n = int(input().strip())
            if n in (3, 4):
                break
            else:
                print("请输入 3 或 4。")
        except ValueError:
            print("请输入整数。")

    print("请输入 4 个坐标（x y 格式，共8个整数，空格或换行分隔）：")
    coords = []
    while len(coords) < 8:
        try:
            line = input().strip()
            if not line:
                continue
            nums = list(map(int, line.split()))
            coords.extend(nums)
        except ValueError:
            print("输入包含非整数，请重新输入当前行。")
            continue
    if len(coords) != 8:
        print("未输入足够的坐标，程序退出。")
        return

    points = [(coords[i], coords[i+1]) for i in range(0, 8, 2)]

    # 输出原始坐标
    print(f"\n{{{format_points(points)},}},")
    # 输出旋转结果
    for angle in (90, 180, 270):
        rotated = rotate_points(points, n, angle)
        print(f"{{{format_points(rotated)},}},")

if __name__ == "__main__":
    main()
