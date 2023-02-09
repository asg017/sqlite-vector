from setuptools import setup

VERSION = "0.0.1-alpha.2"

setup(
    name="datasette-sqlite-vector",
    description="",
    long_description="",
    long_description_content_type="text/markdown",
    author="Alex Garcia",
    url="https://github.com/asg017/sqlite-vector",
    project_urls={
        "Issues": "https://github.com/asg017/sqlite-vector/issues",
        "CI": "https://github.com/asg017/sqlite-vector/actions",
        "Changelog": "https://github.com/asg017/sqlite-vector/releases",
    },
    license="MIT License, Apache License, Version 2.0",
    version=VERSION,
    packages=["datasette_sqlite_vector"],
    entry_points={"datasette": ["sqlite_vector = datasette_sqlite_vector"]},
    install_requires=["datasette", "sqlite-vector"],
    extras_require={"test": ["pytest"]},
    python_requires=">=3.7",
)