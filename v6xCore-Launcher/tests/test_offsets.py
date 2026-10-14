import configparser
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("updater", ROOT / "tools" / "update_offsets.py")
updater = importlib.util.module_from_spec(spec)
spec.loader.exec_module(updater)

class OffsetTests(unittest.TestCase):
    def setUp(self):
        self.globals = (ROOT / "reference" / "offsets.json").read_bytes()
        self.schema = (ROOT / "reference" / "client_dll.json").read_bytes()
        self.meta = json.loads((ROOT / "reference" / "provenance.json").read_text())

    def test_snapshot_hashes(self):
        for name, blob in (("offsets.json", self.globals), ("client_dll.json", self.schema)):
            self.assertEqual(hashlib.sha256(blob).hexdigest(), self.meta["files"][name]["sha256"])

    def test_expected_keys_and_bounds(self):
        c = updater.make_config(self.globals, self.schema, self.meta)
        self.assertEqual(len(c["globals"]), 6)
        self.assertEqual(len(c["schema"]), len(updater.FIELDS) + 1)
        self.assertEqual(c["metadata"]["commit"], self.meta["commit"])
        for value in c["globals"].values():
            self.assertGreater(int(value, 0), 0)
            self.assertLessEqual(int(value, 0), 0x40000000)

    def test_missing_required_field(self):
        schema = json.loads(self.schema)
        del schema["client.dll"]["classes"]["C_BaseEntity"]["fields"]["m_iHealth"]
        with self.assertRaises(KeyError):
            updater.make_config(self.globals, json.dumps(schema).encode(), self.meta)

    def test_invalid_global_is_rejected(self):
        for bad in [0, -1, 0x40000001, True, "123", None, 1.5]:
            with self.subTest(bad=bad):
                data = json.loads(self.globals)
                data["client.dll"]["dwEntityList"] = bad
                with self.assertRaises(ValueError):
                    updater.make_config(json.dumps(data).encode(), self.schema, self.meta)

    def test_invalid_schema_is_rejected(self):
        data = json.loads(self.schema)
        data["client.dll"]["classes"]["C_BaseEntity"]["fields"]["m_iHealth"] = 0x100001
        with self.assertRaises(ValueError):
            updater.make_config(self.globals, json.dumps(data).encode(), self.meta)

    def test_generated_ini_matches_snapshot(self):
        expected = updater.make_config(self.globals, self.schema, self.meta)
        actual = configparser.ConfigParser()
        actual.optionxform = str
        actual.read(ROOT / "offsets.ini")
        for section in ("metadata", "globals", "schema"):
            self.assertEqual(dict(actual[section]), dict(expected[section]))

if __name__ == "__main__":
    unittest.main()
