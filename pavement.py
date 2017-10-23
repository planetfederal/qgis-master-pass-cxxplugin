# -*- coding: utf-8 -*-
#
# (c) 2016 Boundless, http://boundlessgeo.com
# This code is licensed under the GPL 2.0 license.
#
import os
import zipfile
import StringIO
import json
from collections import defaultdict

import requests
from paver.easy import *


options(
    plugin = Bunch(
        name = 'keychainbridge',
        source_dir = path('keychainbridge'),
        package_dir = path('.'),
        excludes = [
            '.git'
        ],
        # skip certain files inadvertently found by exclude pattern globbing
        skip_exclude = []
    ),

    sphinx = Bunch(
        docroot = path('docs'),
        sourcedir = path('docs/source'),
        builddir = path('docs/build')
    )
)


def create_settings_docs(options):
    settings_file = path(options.plugin.name) / "settings.json"
    doc_file = options.sphinx.sourcedir / "settingsconf.rst"
    try:
        with open(settings_file) as f:
            settings = json.load(f)
    except:
        return
    grouped = defaultdict(list)
    for setting in settings:
        grouped[setting["group"]].append(setting)
    with open (doc_file, "w") as f:
        f.write(".. _plugin_settings:\n\n"
                "Plugin settings\n===============\n\n"
                "The plugin can be adjusted using the following settings, "
                "to be found in its settings dialog (|path_to_settings|).\n")
        for groupName, group in grouped.items():
            section_marks = "-" * len(groupName)
            f.write("\n%s\n%s\n\n"
                    ".. list-table::\n"
                    "   :header-rows: 1\n"
                    "   :stub-columns: 1\n"
                    "   :widths: 20 80\n"
                    "   :class: non-responsive\n\n"
                    "   * - Option\n"
                    "     - Description\n"
                    % (groupName, section_marks))
            for setting in group:
                f.write("   * - %s\n"
                        "     - %s\n"
                        % (setting["label"], setting["description"]))


@task
@cmdopts([
    ('clean', 'c', 'clean out built artifacts first'),
    ('sphinx_theme=', 's', 'Sphinx theme to use in documentation'),
])
def builddocs(options):
    try:
        # May fail if not in a git repo
        sh("git submodule init")
        sh("git submodule update")
    except:
        pass
    create_settings_docs(options)
    if getattr(options, 'clean', False):
        options.sphinx.builddir.rmtree()
    if getattr(options, 'sphinx_theme', False):
        # overrides default theme by the one provided in command line
        set_theme = "-D html_theme='{}'".format(options.sphinx_theme)
    else:
        # Uses default theme defined in conf.py
        set_theme = ""
    sh("sphinx-build -a {} {} {}/html".format(set_theme,
                                              options.sphinx.sourcedir,
                                              options.sphinx.builddir))