from odin_data.meta_writer.meta_writer import MetaWriter
from odin_data.meta_writer.hdf5dataset import StringHDF5Dataset
import tempfile
from unittest.mock import MagicMock
import h5py
import pytest
from numpy import dtype


@pytest.fixture()
def meta_writer_with_temp_file():
    temp_file = tempfile.TemporaryFile()
    meta_writer = MetaWriter("", "", MagicMock(), MagicMock)
    meta_writer._hdf5_file = h5py.File(temp_file, "r+")
    return meta_writer


def write_meta_with_dataset(meta_writer: MetaWriter, dataset):
    meta_writer._datasets = {"": dataset}
    meta_writer._create_datasets(1)
    return meta_writer._hdf5_file


def test_when_string_written_then_expected_type(meta_writer_with_temp_file):
    data_set = StringHDF5Dataset("test", length=100)
    meta_file = write_meta_with_dataset(meta_writer_with_temp_file, data_set)
    assert meta_file["test"] is not None
    assert meta_file["test"].dtype == dtype("S100")


def test_given_data_set_with_no_chunks_but_max_shape_then_use_chunks(
    meta_writer_with_temp_file,
):
    data_set = StringHDF5Dataset("test", maxshape=(100,))
    meta_file = write_meta_with_dataset(meta_writer_with_temp_file, data_set)
    assert meta_file["test"].chunks is not None


def test_given_data_set_with_fixed_length_and_no_max_shape_then_has_no_max_shape_or_chunking(
    meta_writer_with_temp_file,
):
    data_set = StringHDF5Dataset("test", maxshape=None, length=100)
    meta_file = write_meta_with_dataset(meta_writer_with_temp_file, data_set)
    assert meta_file["test"].maxshape == (0,)
    assert meta_file["test"].chunks is None


def test_given_data_set_with_fixed_length_and_specified_max_shape_then_has_max_shape_and_chunking(
    meta_writer_with_temp_file,
):
    data_set = StringHDF5Dataset("test", maxshape=(200,), length=100)
    meta_file = write_meta_with_dataset(meta_writer_with_temp_file, data_set)
    assert meta_file["test"].maxshape == (200,)
    assert meta_file["test"].chunks is not None


def test_given_data_set_with_fixed_length_then_can_add_value(
    meta_writer_with_temp_file,
):
    data_set = StringHDF5Dataset("test", maxshape=None, length=100)
    write_meta_with_dataset(meta_writer_with_temp_file, data_set)
    data_set.add_value(10)
    data_set.flush()
