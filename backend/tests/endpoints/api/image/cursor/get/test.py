import ctypes
import os
import requests
import sys
import struct

HOST: str = os.environ["HOST"]
PATH_TO_TESTS_DIR: str = os.environ["PATH_TO_TESTS_DIR"]
THIS_DIR: str = f"{PATH_TO_TESTS_DIR}/endpoints/api/image/cursor/get"


class Batch:
    content: bytes
    offset: ctypes.c_uint64 = ctypes.c_uint64(0)

    def __init__(self, content: bytes):
        self.content = content

    def __str__(self):
        return f"BATCH: offset: {self.offset.value} content: {len(self.content)} bytes"


class BatchItem:
    idx: ctypes.c_uint16
    img_id: ctypes.c_uint64
    img_size: ctypes.c_uint64
    img_content: memoryview

    def __init__(self, batch: Batch):
        self.idx = ctypes.c_uint16(
            struct.unpack_from("<H", batch.content, offset=batch.offset.value)[0]
        )
        batch.offset.value += 2
        self.img_id = ctypes.c_uint64(
            struct.unpack_from("<Q", batch.content, offset=batch.offset.value)[0]
        )
        batch.offset.value += 8
        self.img_size = ctypes.c_uint64(
            struct.unpack_from("<Q", batch.content, offset=batch.offset.value)[0]
        )
        batch.offset.value += 8
        self.img_content = memoryview(batch.content)[
            batch.offset.value : batch.offset.value + self.img_size.value
        ]
        batch.offset.value += self.img_size.value

    def __str__(self):
        return f"BatchItem idx: {self.idx.value} img_id: {self.img_id.value} img_size:{self.img_size.value} img_content:{self.img_content}"


def get_batch(start: int, limit: int) -> Batch:
    url = f"http://{HOST}/api/image/cursor?current={start}&limit={limit}"
    resp = requests.get(url).content
    batch: Batch = Batch(resp)
    print(batch, file=sys.stderr)
    return batch


def extract_item_from_batch(batch: Batch) -> BatchItem:
    return BatchItem(batch)


def extract_all_items_from_batch(batch: Batch) -> list[BatchItem]:
    items: list[BatchItem] = []
    while len(batch.content) > batch.offset.value:
        items.append(extract_item_from_batch(batch))

    return items


def compare_with_file(item: BatchItem, file_path: str) -> bool:
    with open(file_path, "rb") as file:
        content = file.read()
        comp = item.img_content == content
        print(f"Comparing:\n{item} with {file_path} -> {comp}", file=sys.stderr)
        return comp


def main():
    batch = get_batch(4, 4)
    items = extract_all_items_from_batch(batch)
    for item in items:
        if not compare_with_file(item, f"{THIS_DIR}/{item.img_id.value}.webp"):
            print("TEST FAILED")
            return 1

    print("TEST PASSED")
    return 0


if __name__ == "__main__":
    main()
