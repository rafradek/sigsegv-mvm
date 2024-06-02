#!/usr/bin/python
# vim: set sts=2 ts=8 sw=2 tw=99 et:

import sys
from ambuild2 import run

parser = run.BuildParser(sourcePath = sys.path[0], api = '2.2')

parser.options.add_argument('--hl2sdk-root', type=str, dest='hl2sdk_root', default=None, help='Root search folder for HL2SDKs')
parser.options.add_argument('--hl2sdk-manifest-path', type=str, dest='hl2sdk_manifest', default=None, help='Path to HL2SDK Manifests')
parser.options.add_argument('--mms-path', type=str, dest='mms_path', default=None, help='Path to Metamod:Source')
parser.options.add_argument('--sm-path', type=str, dest='sm_path', default=None, help='Path to SourceMod')
parser.options.add_argument('--enable-debug', action='store_const', const='1', dest='debug', help='Enable debugging symbols')
parser.options.add_argument('--enable-optimize', action='store_const', const='1', dest='opt', help='Enable optimization')
parser.options.add_argument('--enable-experimental', action='store_const', const='1', dest='experimental', help='Enable experimental/incomplete features')
parser.options.add_argument('--exclude-mods-debug', action='store_const', const='1', dest='exclude_mods_debug', help='Don\'t compile any mods in the Debug group')
parser.options.add_argument('--exclude-mods-visualize', action='store_const', const='1', dest='exclude_mods_visualize', help='Don\'t compile any mods in the Visualize group')
parser.options.add_argument('--exclude-mods-mvm', action='store_const', const='1', dest='exclude_mods_mvm', help='Don\'t compile any mods in the MvM group')
parser.options.add_argument('--optimize-mods-only', action='store_const', const='1', dest='optimize_mods_only', help='Only compile optimize mods')
parser.options.add_argument('--build-all', action='store_const', const='1', dest='build_all', help='Build additional optimize-mods and no-mvm packages')
parser.options.add_argument('--exclude-vgui', action='store_const', const='1', dest='exclude_mods_vgui', help='Don\'t compile any mods in the VGUI group')
parser.options.add_argument('-s', '--sdks', default='all', dest='sdks', help='Build against specified SDKs; valid args are "all", "present", or comma-delimited list of engine names (default: %default)')
parser.options.add_argument('--targets', type=str, dest='targets', default=None, help="Override the target architecture (use commas to separate multiple targets).")

parser.Configure()
