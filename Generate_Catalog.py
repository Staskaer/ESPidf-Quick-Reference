raw_text = """# 这是什么？

目前用了到esp-idf，但是API不太好记，而且官方例程很长，所以就基于例程把大部分常用的API提取出来，方便自己查阅。

---

# 目录
"""

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

with open("README.md", 'w', encoding="UTF-8") as f:
    f.write(raw_text+"\n\n")
    for title, title_level in title_list:
        f.write("  " * (title_level - 1) + "* [" + title + "](./Reference.md#" + title.replace(" ", "-").replace(":", "").replace("?", "").replace("!", "").replace(".", "").
                replace(",", "").replace(";", "").replace("(", "").replace(")", "").replace("/", "").replace("\\", "").replace("'", "").replace('"', "").lower() + ")\n")
