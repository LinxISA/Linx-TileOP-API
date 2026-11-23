import argparse
import concurrent.futures
import html
import json
import re
import subprocess
from pathlib import Path
from urllib.parse import quote, unquote, urlsplit


def render(directory, destination):
    checkout = directory.parent.parent
    lock = json.loads((checkout / 'contracts/pto-isa-0.59.0-sources.json').read_text())
    operations = json.loads((checkout / 'contracts' / lock['active']['catalog']).read_text())['operations']
    active_names = {op['name'] for op in operations}
    block_operations = ('TIMG2COL',)
    titles = {'variants_zh': '变体索引', 'helpers_zh': '辅助接口'}
    pages = [directory / 'README.md', *(directory / f'{key}.md' for key in titles)]
    pages.extend(directory / 'ops' / f'{name}_zh.md' for name in sorted(active_names.union(block_operations)))
    keys = {page: ('home' if page.name == 'README.md' else page.stem.removesuffix('_zh')) for page in pages}
    index = pages[0].read_text()
    groups = []
    for section in re.split(r'^### ', index, flags=re.M)[1:]:
        title, _, body = section.partition('\n')
        entries = re.findall(r'^\| \[([A-Z0-9_]+)\]\([^\n]+?\) \| `([^`]+)` \| (.*?) \|$', body, re.M)
        groups.append((title, entries))
    if {name for _, entries in groups for name, _, _ in entries} != active_names:
        raise ValueError('README does not match the locked operation catalog')
    variants = {}
    for variant, parent in re.findall(r'^\| `([A-Z0-9_]+)` \| \[([A-Z0-9_]+)\]\(', (directory / 'variants_zh.md').read_text(), re.M):
        variants.setdefault(parent, []).append(variant)

    def link(name, label, metadata=''):
        return f'<li><a href="#{name}" data-search="{html.escape(metadata, quote=True)}">{html.escape(label)}</a></li>'

    sidebar = '<ul class="overview">' + link('home', '操作总览')
    sidebar += ''.join(link(key.removesuffix('_zh'), title) for key, title in titles.items()) + '</ul>'
    for title, entries in groups:
        sidebar += f'<details class="category"><summary>{html.escape(title)}</summary><ul>'
        for name, engine, description in entries:
            sidebar += link(name, name, ' '.join([engine, description, *variants.get(name, [])]))
        sidebar += '</ul></details>'
    sidebar += '<details class="category"><summary>块操作</summary><ul>'
    sidebar += ''.join(link(name, name, ' '.join(variants.get(name, []))) for name in block_operations) + '</ul></details>'

    def read(page):
        key = keys[page]
        body = subprocess.check_output(['pandoc', '-f', 'gfm-raw_html', '-t', 'html5', '--no-highlight', f'--id-prefix={key}--', str(page)], text=True)
        body = re.sub(r'<table>(.*?)</table>', r'<div class="table-wrap" tabindex="0"><table>\1</table></div>', body, flags=re.S)

        def rewrite(match):
            href = html.unescape(match[1])
            url = urlsplit(href)
            if url.scheme or not url.path:
                return match[0]
            target = (page.parent / unquote(url.path)).resolve()
            if target in keys:
                fragment = quote(unquote(url.fragment), safe='-_')
                href = '#' + keys[target] + ('--' + fragment if fragment else '')
            elif target == directory / 'index.html':
                return '' if page.name == 'README.md' else 'href="#home"'
            else:
                raise ValueError(f'{page.name}: relative link outside the reference: {href}')
            return 'href="' + html.escape(href, quote=True) + '"'

        body = re.sub(r'href="([^"]+)"', rewrite, body).replace(' · <a >离线 HTML</a>', '')
        sections = []
        for heading, label in re.findall(r'<h2 id="([^"]+)">(.*?)</h2>', body, re.S):
            sections.append(f'<a href="#{html.escape(heading, quote=True)}">{label}</a>')
        nav = '<nav class="sections" aria-label="本页目录">' + ''.join(sections) + '</nav>'
        body = body.replace('</h1>', '</h1>' + nav, 1) if key != 'home' else body
        return f'<article id="{key}" data-title="{html.escape(titles.get(page.stem, key), quote=True)}">\n{body}</article>'

    with concurrent.futures.ThreadPoolExecutor(max_workers=8) as workers:
        content = '\n'.join(workers.map(read, pages))
    template = Path(__file__).with_name('site.html').read_text()
    destination.write_text(template.replace('@@SIDEBAR@@', sidebar).replace('@@PAGES@@', content), encoding='utf-8')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Render the TileOP reference as one offline HTML file (requires Pandoc).')
    parser.add_argument('--output', type=Path, default=Path(__file__).with_name('index.html'))
    args = parser.parse_args()
    render(Path(__file__).resolve().parent, args.output)
