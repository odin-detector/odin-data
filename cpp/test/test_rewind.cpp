#include <iostream>
#include <vector>
#include <hdf5.h>
#include <hdf5_hl.h>

int main(int argc, char *argv[]) {
  hsize_t CHUNK_NX = 704;
  hsize_t CHUNK_NY = 1484;
  size_t buf_size = CHUNK_NX * CHUNK_NY * sizeof(int16_t);
  int16_t data_buf[CHUNK_NY][CHUNK_NX];
  int16_t one_buf[CHUNK_NY][CHUNK_NX];
  uint32_t filter_mask = 0;
  hid_t dtype = H5T_NATIVE_UINT16;

  static const hsize_t dims[] = {1, 1484, 1408};
  std::vector<hsize_t> dset_dims(dims, dims + sizeof(dims)/ sizeof(dims[0]));
  std::vector<hsize_t> max_dims = dset_dims;
  max_dims[0] = H5S_UNLIMITED;

  /* Create the data space */
  hid_t dataspace = H5Screate_simple(dset_dims.size(), &dset_dims.front(), &max_dims.front());

  /* Create a new file */
  hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
  H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);
  hid_t file = H5Fcreate("/tmp/rewind.hdf5", H5F_ACC_TRUNC, H5P_DEFAULT, fapl);

  /* Create properties */
  hid_t prop = H5Pcreate(H5P_DATASET_CREATE);
  char fill_value[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  H5Pset_fill_value(prop, dtype, fill_value);
  static const hsize_t cdims[] = {1, CHUNK_NY, CHUNK_NX};
  std::vector<hsize_t> chunk_dims(cdims, cdims + sizeof(cdims)/ sizeof(cdims[0]));
  H5Pset_chunk(prop, dset_dims.size(), &chunk_dims.front());
  hid_t dapl = H5Pcreate(H5P_DATASET_ACCESS);

  hid_t dset_id = H5Dcreate2(file, "data", dtype, dataspace, H5P_DEFAULT, prop, dapl);

  /* Initialize data for one chunk */
  int i, n, j;
  for (i = n = 0; i < CHUNK_NY; i++) {
    for (j = 0; j < CHUNK_NX; j++) {
      data_buf[i][j] = n++;
      one_buf[i][j] = 1;
    }
  }

  /* Write chunk data using the direct write function. */
  static const hsize_t offst[] = {0, 0, 0};
  std::vector<hsize_t> offset(offst, offst + sizeof(offst)/ sizeof(offst[0]));
  H5DOwrite_chunk(dset_id, H5P_DEFAULT, filter_mask, &offset.front(), buf_size, data_buf);

  /* Write second chunk. */
  offset[2] = CHUNK_NX;
  H5DOwrite_chunk(dset_id, H5P_DEFAULT, filter_mask, &offset.front(), buf_size, data_buf);

  /* Overwrite first chunk. */
  offset[2] = 0;
  H5DOwrite_chunk(dset_id, H5P_DEFAULT, filter_mask, &offset.front(), buf_size, one_buf);

  H5Fclose(file);

  return 0;
}
