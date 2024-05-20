#!/bin/bash

cd build/release
ambuild

pushd package
	(
		rm -f ../package-linux.zip
		type zip >/dev/null 2>&1 && zip -y -q -FSr ../package-linux.zip *
	) &
popd

pushd package-no-mvm
	(
		rm -f ../package-linux-no-mvm.zip
		type zip >/dev/null 2>&1 && zip -y -q -FSr ../package-linux-no-mvm.zip *
	) &
popd

pushd package-optimize-only
	(
		rm -f ../package-linux-optimize-only.zip
		type zip >/dev/null 2>&1 && zip -y -q -FSr ../package-linux-optimize-only.zip *
	) &
popd

wait