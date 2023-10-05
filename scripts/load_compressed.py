#!/usr/bin/env python3

#
# Copyright 2023 Adobe. All rights reserved.
# This file is licensed to you under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License. You may obtain a copy
# of the License at http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software distributed under
# the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR REPRESENTATIONS
# OF ANY KIND, either express or implied. See the License for the specific language
# governing permissions and limitations under the License.
#
# -*- coding: utf-8 -*-

from pathlib import Path

import msgpack
import zstd
from rich import print


def load_compressed(filename: Path):
    data = msgpack.loads(zstd.loads(filename.read_bytes()))
    print(data)


def main():
    data_dir = Path(__file__).parents[1] / "data"
    load_compressed(data_dir / "test/problem_compressed.zst")


if __name__ == "__main__":
    main()
