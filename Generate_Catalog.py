

title_list = []


with open('Reference.md', 'r', encoding="UTF-8") as f:
    while True:
        line = f.readline()
        if not line:
            break
        title_level = line.count('#')
        if title_level > 0:
            if "include" in line:
                continue
            title = line.strip('#').strip()
            title_list.append((title, title_level))

with open("README.md", 'r+', encoding="UTF-8") as f:
    # 先定位到目录行再写入
    while True:
        line = f.readline()
        if not line:
            break
        if "目录" in line:
            f.seek(f.tell())
            break
    f.write("\n\n")
    for title, title_level in title_list:
        f.write("  " * (title_level - 1) + "* [" + title + "](./Reference.md#" + title.replace(" ", "-").replace(":", "").replace("?", "").replace("!", "").replace(".", "").
                replace(",", "").replace(";", "").replace("(", "").replace(")", "").replace("/", "").replace("\\", "").replace("'", "").replace('"', "").lower() + ")\n")
