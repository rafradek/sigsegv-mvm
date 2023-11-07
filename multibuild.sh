#!/bin/bash

pushd build/release
	(
		ambuild
		cd package
		rm -f ../package-linux.zip
		type zip >/dev/null 2>&1 && zip -y -q -FSr ../package-linux.zip *
	)
popd

pushd build/release/optimize-only
	(
		ambuild
		cd package
		rm -f ../../package-linux-optimize-only.zip
		type zip >/dev/null 2>&1 && zip -y -q -FSr ../../package-linux-optimize-only.zip *
	) &
popd

pushd build/release/no-mvm
	(
		ambuild
		cd package
		rm -f ../../package-linux-no-mvm.zip
		type zip >/dev/null 2>&1 && zip -y -q -FSr ../../package-linux-no-mvm.zip *
	) &
popd

wait