import argparse
from pathlib import Path


def render(source):
    html = source.read_text(encoding='utf-8')
    replacements = {
        '</head>': '  <link rel="stylesheet" href="feedback.css">\n</head>',
        '<main id="content">': '''<main id="content">
      <nav class="feedback" aria-label="文档反馈">
        <a id="feedback-page" href="https://github.com/LinxISA/Linx-TileOP-API/issues/new?title=%5BTileOP%20%E6%96%87%E6%A1%A3%5D%5B0.59%5D%20%E6%93%8D%E4%BD%9C%E6%80%BB%E8%A7%88">反馈本页</a>
        <a href="https://github.com/LinxISA/Linx-TileOP-API/issues?q=is%3Aissue+in%3Atitle+%22TileOP+%E6%96%87%E6%A1%A3%22+%220.59%22">查看已提交的意见</a>
      </nav>''',
        '</body>': '  <script src="feedback.js"></script>\n</body>',
    }
    for marker, replacement in replacements.items():
        if html.count(marker) != 1:
            raise ValueError(f'Expected one {marker!r} entry in the offline HTML')
        html = html.replace(marker, replacement, 1)
    return html


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Add the GitHub feedback links to the 0.59 offline reference.')
    parser.add_argument('source', type=Path)
    args = parser.parse_args()
    output = Path(__file__).resolve().parent / 'docs' / 'index.html'
    output.write_text(render(args.source), encoding='utf-8')
