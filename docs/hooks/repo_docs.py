"""Serve Markdown files from anywhere in the repository without copying them.

mkdocs only looks at ``docs_dir``. The module READMEs, the ADRs and the other
Markdown files are kept next to the code they describe, so this hook adds the
files listed under ``extra.repo_docs`` in mkdocs.yml to the site at their
repository path (firmware/config/README.md becomes /firmware/config/).

All Markdown files link to each other with paths relative to the repository,
the way GitHub renders them. Before a page is rendered, those links are
rewritten for the site: a link to another page becomes a link to that page,
and a link to any other file (sources, headers, scripts) points to the file on
GitHub.

Notes are written as GitHub alerts (``> [!NOTE]``), which GitHub renders; the
hook turns them into Material admonitions for the site.
"""

import os
import posixpath
import re

from mkdocs.structure.files import File

_LINK = re.compile(r'(!?)\[([^\]]*)\]\(([^)\s]+)((?:\s+"[^"]*")?)\)')

# GitHub alert -> Material admonition type and title
_ALERTS = {
    "NOTE": ("note", "Note"),
    "TIP": ("tip", "Tip"),
    "IMPORTANT": ("info", "Important"),
    "WARNING": ("warning", "Warning"),
    "CAUTION": ("danger", "Caution"),
}
_ALERT = re.compile(r"^> \[!(NOTE|TIP|IMPORTANT|WARNING|CAUTION)\][ \t]*\n((?:>.*\n?)*)", re.M)

# repository path -> File, for every page on the site
_pages = {}


def _repo_root(config):
    return os.path.dirname(config.config_file_path)


def _repo_path(file, config):
    return os.path.relpath(file.abs_src_path, _repo_root(config)).replace(os.sep, "/")


def on_files(files, config):
    root = _repo_root(config)
    for path in config.extra.get("repo_docs", []):
        if files.get_file_from_path(path) is not None:
            continue
        files.append(
            File(
                path,
                src_dir=root,
                dest_dir=config.site_dir,
                use_directory_urls=config.use_directory_urls,
            )
        )
    _pages.clear()
    for file in files.documentation_pages():
        repo_path = _repo_path(file, config)
        file.edit_uri = repo_path  # "edit this page" needs the repository path
        _pages[repo_path] = file
    return files


def _alert_to_admonition(match):
    kind, title = _ALERTS[match.group(1)]
    lines = match.group(2).splitlines()
    body = "".join("    " + line[2:] + "\n" if line.startswith("> ") else "\n" for line in lines)
    return f'!!! {kind} "{title}"\n{body}'


def on_page_markdown(markdown, page, config, files):
    markdown = _ALERT.sub(_alert_to_admonition, markdown)
    root = _repo_root(config)
    page_dir = posixpath.dirname(_repo_path(page.file, config))
    src_dir = posixpath.dirname(page.file.src_uri)
    branch = config.extra.get("repo_branch", "main")

    def rewrite(match):
        bang, text, url, title = match.groups()
        if "://" in url or url.startswith(("#", "mailto:")):
            return match.group(0)
        target, _, fragment = url.partition("#")
        fragment = f"#{fragment}" if fragment else ""
        repo_target = posixpath.normpath(posixpath.join(page_dir, target))
        if repo_target in _pages:
            # Relative to the source file, so mkdocs can still validate it.
            new = posixpath.relpath(_pages[repo_target].src_uri, src_dir or ".")
            return f"{bang}[{text}]({new}{fragment}{title})"
        abs_target = os.path.join(root, repo_target)
        if os.path.isdir(abs_target):
            kind = "tree"
        elif os.path.isfile(abs_target):
            kind = "raw" if bang else "blob"
        else:
            return match.group(0)  # unknown target, mkdocs warns about it
        new = f"{config.repo_url.rstrip('/')}/{kind}/{branch}/{repo_target}"
        return f"{bang}[{text}]({new}{fragment}{title})"

    return _LINK.sub(rewrite, markdown)
