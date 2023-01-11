#!/bin/bash

pushd build/release
	ambuild &
popd

pushd build/release/optimize-only
	ambuild &
popd

pushd build/release/no-mvm
	ambuild &
popd

wait