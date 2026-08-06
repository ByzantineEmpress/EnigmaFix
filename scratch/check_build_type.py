with open("CMakeCache.txt", "r", encoding="utf-8", errors="ignore") as f:
    for line in f:
        if "CMAKE_BUILD_TYPE" in line:
            print(line.strip())
