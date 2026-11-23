# PTO-C TileOP 接口参考 · GitHub Pages

本分支发布 [0.59 在线手册](https://linxisa.github.io/Linx-TileOP-API/)。各接口页的“反馈本页”会打开同仓库的 GitHub Issue 表单，填入版本、接口名和当前章节链接；读者登录 GitHub 后确认提交。使用 `[TileOP 文档][0.59]` 标题前缀跟踪反馈。

## 更新手册

1. 在 `0.59` 分支的 `docs/tileop-reference/` 中维护 Markdown。使用页面反馈里附带的操作名定位 `ops/<操作名>_zh.md`；总览、变体和辅助接口分别在 `README.md`、`variants_zh.md`、`helpers_zh.md`。
2. 在检出的 `0.59` 源码中运行 `python3 docs/tileop-reference/build_html.py --output /tmp/tileop-059.html`（需要 Pandoc）。也可以使用已生成的新版单文件手册。
3. 在 `gh-pages` 分支的根目录运行 `python3 build_pages.py /tmp/tileop-059.html`，提交更新后的 `docs/index.html` 并推送至 `gh-pages`。GitHub Pages 从该分支的 `/docs` 自动发布。

首次发布使用用户提供的 0.59 HTML。发布脚本会保留手册正文、搜索和锚点，仅添加在线反馈所需的资源和入口。
