

title_list = []


with open('Reference.md', 'r', encoding="UTF-8") as f:
    while True:
        line = f.readline()
        if not line:
            break
        title_level = line.count('#')
        if title_level > 0:
            if "include" in line or "define" in line or "if" in line or "else" in line:
                continue
            title = line.strip('#').strip()
            title_list.append((title, title_level))

with open("README.md", 'r+', encoding="UTF-8") as f:
    # 先定位到目录行再写入
    while True:
        line = f.readline()
        if not line:
            break
        if "目录" in line[:5]:
            f.seek(f.tell())
            f.truncate()
            break
    f.write("\n\n")
    for title, title_level in title_list:
        f.write("  " * (title_level - 1) + "* [" + title + "](./Reference.md#" + title.replace(" ", "-").replace(":", "").replace("?", "").replace("!", "").replace(".", "").
                replace(",", "").replace(";", "").replace("(", "").replace(")", "").replace("/", "").replace("\\", "").replace("'", "").replace('"', "").lower() + ")\n")
