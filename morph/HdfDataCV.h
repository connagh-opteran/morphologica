/*
 * Additional OpenCV-aware HdfData saving and loading functions. You'll need the OpenCV
 * headers for this, and probably you'll need to link to some of the OpenCV libraries
 * too.
 */
#include <morph/HdfData.h>
#include <opencv2/core/types.hpp>
#include <opencv2/core/mat.hpp>

namespace morph {

    class HdfDataCV : public HdfData
    {
        HdfDataCV (const std::string fname,
                   const FileAccess _file_access = FileAccess::TruncateWrite,
                   const bool show_hdf_internal_errors = false)
        {
            this->init (fname, _file_access, show_hdf_internal_errors);
        }

        HdfDataCV (const std::string fname, const bool read_data, const bool show_hdf_internal_errors = false)
        {
            FileAccess _file_access = (read_data ? FileAccess::ReadOnly : FileAccess::TruncateWrite);
            this->init (fname, _file_access, show_hdf_internal_errors);
        }

        ~HdfDataCV()
        {
            herr_t status = H5Fclose (this->file_id);
            if (status) { std::cerr << "Error closing HDF5 file; status: " << status << std::endl; }
        }

        //! Add a cv::Point_<T> to the HDF5 file
        template <typename T>
        void add_contained_vals (const char* path, const cv::Point_<T>& val)
        {
            this->process_groups (path);
            hsize_t dim_vec2dcoord[2];
            dim_vec2dcoord[0] = 1;
            dim_vec2dcoord[1] = 2; // 2 coordinates in a cv::Point_
            // Note 2 dims (1st arg, which is rank = 2)
            hid_t dataspace_id = H5Screate_simple (2, dim_vec2dcoord, NULL);
            // Now determine width of T and select the most suitable data type to pass to open_dataset()
            hid_t dataset_id = 0;
            herr_t status = 0;
            // Copy the data out of the point and into a nice contiguous array
            std::vector<T> data_array(2, 0);
            data_array[0] = val.x;
            data_array[1] = val.y;
            if constexpr (std::is_same<std::decay_t<T>, double>::value == true) {
                dataset_id = this->open_dataset (path, H5T_IEEE_F64LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, 1, 2);
                status = H5Dwrite (dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(data_array[0]));

            } else if constexpr (std::is_same<std::decay_t<T>, float>::value == true) {
                dataset_id = this->open_dataset (path, H5T_IEEE_F64LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, 1, 2);
                status = H5Dwrite (dataset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(data_array[0]));

            } else if constexpr (std::is_same<std::decay_t<T>, int>::value == true) {
                dataset_id = this->open_dataset (path, H5T_STD_I64LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, 1, 2);
                status = H5Dwrite (dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(data_array[0]));

            } else if constexpr (std::is_same<std::decay_t<T>, long long int>::value == true) {
                dataset_id = this->open_dataset (path, H5T_STD_I64LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, 1, 2);
                status = H5Dwrite (dataset_id, H5T_NATIVE_LLONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(data_array[0]));

            } else if constexpr (std::is_same<std::decay_t<T>, unsigned int>::value == true) {
                dataset_id = this->open_dataset (path, H5T_STD_U64LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, 1, 2);
                status = H5Dwrite (dataset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(data_array[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, unsigned long long int>::value == true) {
                dataset_id = this->open_dataset (path, H5T_STD_U64LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, 1, 2);
                status = H5Dwrite (dataset_id, H5T_NATIVE_ULLONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(data_array[0]));

            } else {
                throw std::runtime_error ("HdfData::add_contained_vals: Don't know how to store a cv::Point_ of that type");
            }
            this->handle_error (status, "Error. status after H5Dwrite 8: ");
            status = H5Dclose (dataset_id);
            this->handle_error (status, "Error. status after H5Dclose: ");
            status = H5Sclose (dataspace_id);
            this->handle_error (status, "Error. status after H5Sclose: ");
        }

        //! Write out cv::Mat, along with the data type and the channels as metadata.
        void add_contained_vals (const char* path, const cv::Mat& vals)
        {
            this->process_groups (path);

            hsize_t dim_mat[2]; // 2 dimensions supported (even though Mat's can do n dimensions)

            cv::Size ms = vals.size();
            dim_mat[0] = ms.height;
            int channels = vals.channels();
            if (channels > 4) {
                // error, can't handle >4 channels
            }
            dim_mat[1] = ms.width * channels;

            hid_t dataspace_id = H5Screate_simple (2, dim_mat, NULL);

            hid_t dataset_id;
            herr_t status;

            int cv_type = vals.type();

            switch (cv_type) {

            case CV_8UC1:
            case CV_8UC2:
            case CV_8UC3:
            case CV_8UC4:
            {
                dataset_id = this->open_dataset (path, H5T_STD_U8LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, dim_mat[0], dim_mat[1]);
                status = H5Dwrite (dataset_id, H5T_NATIVE_UCHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT, vals.data);
                break;
            }
            case CV_8SC1:
            case CV_8SC2:
            case CV_8SC3:
            case CV_8SC4:
            {
                dataset_id = this->open_dataset (path, H5T_STD_I8LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, dim_mat[0], dim_mat[1]);
                status = H5Dwrite (dataset_id, H5T_NATIVE_CHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT, vals.data);
                break;
            }
            case CV_16UC1:
            case CV_16UC2:
            case CV_16UC3:
            case CV_16UC4:
            {
                dataset_id = this->open_dataset (path, H5T_STD_U16LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, dim_mat[0], dim_mat[1]);
                status = H5Dwrite (dataset_id, H5T_NATIVE_USHORT, H5S_ALL, H5S_ALL, H5P_DEFAULT, vals.data);
                break;
            }
            case CV_16SC1:
            case CV_16SC2:
            case CV_16SC3:
            case CV_16SC4:
            {
                dataset_id = this->open_dataset (path, H5T_STD_I16LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, dim_mat[0], dim_mat[1]);
                status = H5Dwrite (dataset_id, H5T_NATIVE_SHORT, H5S_ALL, H5S_ALL, H5P_DEFAULT, vals.data);
                break;
            }
            case CV_32SC1:
            case CV_32SC2:
            case CV_32SC3:
            case CV_32SC4:
            {
                dataset_id = this->open_dataset (path, H5T_STD_I32LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, dim_mat[0], dim_mat[1]);
                status = H5Dwrite (dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, vals.data);
                break;
            }
            case CV_32FC1:
            case CV_32FC2:
            case CV_32FC3:
            case CV_32FC4:
            {
                dataset_id = this->open_dataset (path, H5T_IEEE_F32LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, dim_mat[0], dim_mat[1]);
                status = H5Dwrite (dataset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, vals.data);
                break;
            }
            case CV_64FC1:
            case CV_64FC2:
            case CV_64FC3:
            case CV_64FC4:
            {
                dataset_id = this->open_dataset (path, H5T_IEEE_F64LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, dim_mat[0], dim_mat[1]);
                status = H5Dwrite (dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, vals.data);
                break;
            }
            default:
            {
                //cerr << "Unknown CvType " << cv_type << endl;
                dataset_id = -1; // What's correct for an error here?
                break;
            }
            }

            this->handle_error (status, "Error. status after H5Dwrite 9: ");
            status = H5Dclose (dataset_id);
            this->handle_error (status, "Error. status after H5Dclose: ");
            status = H5Sclose (dataspace_id);
            this->handle_error (status, "Error. status after H5Sclose: ");

            // Last, write the type in a special metadata field.
            std::string pathtype (path);
            pathtype += "_type";
            this->add_val (pathtype.c_str(), cv_type);
            pathtype = std::string(path) + "_channels";
            this->add_val (pathtype.c_str(), channels);
        }

        /*!
         * Add an array of T, where T may be various 2D coordinates or just scalar
         * values. Unfortunately, this looks very similar to the following template for
         * Container<T, Allocator> instead of std::array
         */
        template<typename T, size_t N>
        void add_contained_vals (const char* path, const std::array<T, N>& vals)
        {
            this->process_groups (path);
            hid_t dataspace_id = 0;
            if constexpr (std::is_same<std::decay_t<T>, cv::Point2i>::value == true
                          || std::is_same<std::decay_t<T>, cv::Point2f>::value == true
                          || std::is_same<std::decay_t<T>, cv::Point2d>::value == true
                          || std::is_same<std::decay_t<T>, std::array<float, 2>>::value == true
                          || std::is_same<std::decay_t<T>, std::array<double, 2>>::value == true
                          || std::is_same<std::decay_t<T>, std::pair<float, float>>::value == true
                          || std::is_same<std::decay_t<T>, std::pair<double, double>>::value == true
                          || std::is_same<std::decay_t<T>, std::pair<int, int>>::value == true
                          || std::is_same<std::decay_t<T>, std::pair<unsigned int, unsigned int>>::value == true
                          || std::is_same<std::decay_t<T>, std::pair<long long int, long long int>>::value == true
                          || std::is_same<std::decay_t<T>, std::pair<unsigned long long int, unsigned long long int>>::value == true) {
                hsize_t dim_vec2dcoords[2]; // 2 Dims
                dim_vec2dcoords[0] = N;
                dim_vec2dcoords[1] = 2; // 2 ints in each cv::Point
                // Note 2 dims (1st arg, which is rank = 2)
                dataspace_id = H5Screate_simple (2, dim_vec2dcoords, NULL);
            } else {
                hsize_t dim_singleparam[1];
                dim_singleparam[0] = N;
                dataspace_id = H5Screate_simple (1, dim_singleparam, NULL);
            }

            hid_t dataset_id = 0;
            herr_t status = 0;
            if constexpr (std::is_same<std::decay_t<T>, double>::value == true) {
                dataset_id = this->open_dataset (path, H5T_IEEE_F64LE, dataspace_id);
                this->check_dataset_space_1_dim (dataset_id, N);
                status = H5Dwrite (dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(vals[0]));

            } else if constexpr (std::is_same<std::decay_t<T>, float>::value == true) {
                dataset_id = this->open_dataset (path, H5T_IEEE_F64LE, dataspace_id);
                this->check_dataset_space_1_dim (dataset_id, N);
                status = H5Dwrite (dataset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(vals[0]));

            } else if constexpr (std::is_same<std::decay_t<T>, int>::value == true) {
                dataset_id = this->open_dataset (path, H5T_STD_I64LE, dataspace_id);
                this->check_dataset_space_1_dim (dataset_id, N);
                status = H5Dwrite (dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(vals[0]));

            } else if constexpr (std::is_same<std::decay_t<T>, long long int>::value == true) {
                dataset_id = this->open_dataset (path, H5T_STD_I64LE, dataspace_id);
                this->check_dataset_space_1_dim (dataset_id, N);
                status = H5Dwrite (dataset_id, H5T_NATIVE_LLONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(vals[0]));

            } else if constexpr (std::is_same<std::decay_t<T>, unsigned int>::value == true) {
                dataset_id = this->open_dataset (path, H5T_STD_U64LE, dataspace_id);
                this->check_dataset_space_1_dim (dataset_id, N);
                status = H5Dwrite (dataset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(vals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, unsigned long long int>::value == true) {
                dataset_id = this->open_dataset (path, H5T_STD_U64LE, dataspace_id);
                this->check_dataset_space_1_dim (dataset_id, N);
                status = H5Dwrite (dataset_id, H5T_NATIVE_ULLONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(vals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, cv::Point2i>::value == true) {
                dataset_id = this->open_dataset (path, H5T_STD_I64LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, N, 2);
                status = H5Dwrite (dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(vals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, cv::Point2d>::value == true) {
                dataset_id = this->open_dataset (path, H5T_IEEE_F64LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, N, 2);
                status = H5Dwrite (dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(vals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, cv::Point2f>::value == true) {
                dataset_id = this->open_dataset (path, H5T_IEEE_F64LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, N, 2);
                status = H5Dwrite (dataset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(vals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, std::array<float,2>>::value == true) {
                dataset_id = this->open_dataset (path, H5T_IEEE_F64LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, N, 2);
                status = H5Dwrite (dataset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(vals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, std::array<double,2>>::value == true) {
                dataset_id = this->open_dataset (path, H5T_IEEE_F64LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, N, 2);
                status = H5Dwrite (dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(vals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, std::pair<float, float>>::value == true) {
                dataset_id = this->open_dataset (path, H5T_IEEE_F64LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, N, 2);
                status = H5Dwrite (dataset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(vals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, std::pair<double, double>>::value == true) {
                dataset_id = this->open_dataset (path, H5T_IEEE_F64LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, N, 2);
                status = H5Dwrite (dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(vals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, std::pair<int, int>>::value == true) {
                dataset_id = this->open_dataset (path, H5T_STD_I64LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, N, 2);
                status = H5Dwrite (dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(vals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, std::pair<unsigned int, unsigned int>>::value == true) {
                dataset_id = this->open_dataset (path, H5T_STD_U64LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, N, 2);
                status = H5Dwrite (dataset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(vals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, std::pair<long long int, long long int>>::value == true) {
                dataset_id = this->open_dataset (path, H5T_STD_I64LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, N, 2);
                status = H5Dwrite (dataset_id, H5T_NATIVE_LLONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(vals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, std::pair<unsigned long long int, unsigned long long int>>::value == true) {
                dataset_id = this->open_dataset (path, H5T_STD_U64LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, N, 2);
                status = H5Dwrite (dataset_id, H5T_NATIVE_ULLONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(vals[0]));

            } else {
                throw std::runtime_error ("HdfData::add_contained_vals<T, N>: Don't know how to store that type");
            }
            this->handle_error (status, "Error. status after H5Dwrite 3: ");
            status = H5Dclose (dataset_id);
            this->handle_error (status, "Error. status after H5Dclose: ");
            status = H5Sclose (dataspace_id);
            this->handle_error (status, "Error. status after H5Sclose: ");
        }

        /*!
         * Makes necessary calls to add a container of values to an HDF5 file store,
         * using path as the name of the variable. This might be a candidate for
         * templating, where it wasn't worth it for add_val(), because of the
         * possibility of vectors, lists, sets, deques etc, of floats, doubles,
         * integers, unsigned integers, etc. However, I can't see the solution to the
         * problem of the specialisation required in just one line of each function, so
         * it may be that may overloaded copies of these functions is still the best
         * solution. It also compiles to linkable, unchanging code in libmorphologica,
         * rather than being header-only,
         */
        template < template <typename, typename> typename Container,
                   typename T,
                   typename Allocator=std::allocator<T> >
        void add_contained_vals (const char* path, const Container<T, Allocator>& vals)
        {
            if (vals.empty()) { return; }
            this->process_groups (path);

            hid_t dataspace_id = 0;
            // Compile time logic to determine if the contained data has 2 elements
            if constexpr (std::is_same<std::decay_t<T>, cv::Point2i>::value == true
                          || std::is_same<std::decay_t<T>, cv::Point2f>::value == true
                          || std::is_same<std::decay_t<T>, cv::Point2d>::value == true
                          || std::is_same<std::decay_t<T>, std::array<float, 2>>::value == true
                          || std::is_same<std::decay_t<T>, std::array<double, 2>>::value == true
                          || std::is_same<std::decay_t<T>, std::pair<float, float>>::value == true
                          || std::is_same<std::decay_t<T>, std::pair<double, double>>::value == true
                          || std::is_same<std::decay_t<T>, std::pair<int, int>>::value == true
                          || std::is_same<std::decay_t<T>, std::pair<unsigned int, unsigned int>>::value == true
                          || std::is_same<std::decay_t<T>, std::pair<long long int, long long int>>::value == true
                          || std::is_same<std::decay_t<T>, std::pair<unsigned long long int, unsigned long long int>>::value == true) {
                hsize_t dim_vec2dcoords[2]; // 2 Dims
                dim_vec2dcoords[0] = vals.size();
                dim_vec2dcoords[1] = 2; // 2 ints in each cv::Point
                // Note 2 dims (1st arg, which is rank = 2)
                dataspace_id = H5Screate_simple (2, dim_vec2dcoords, NULL);
            } else {
                hsize_t dim_singleparam[1];
                dim_singleparam[0] = vals.size();
                dataspace_id = H5Screate_simple (1, dim_singleparam, NULL);
            }

            // A container suitable for holding the values to be written to the HDF5.
            // vals is copied into outvals, because if vals is an std::list, it has no
            // [] operator.
            std::vector<T> outvals(vals.size());
            std::copy (vals.begin(), vals.end(), outvals.begin());

            hid_t dataset_id = 0;
            herr_t status = 0;
            if constexpr (std::is_same<std::decay_t<T>, double>::value == true) {
                dataset_id = this->open_dataset (path, H5T_IEEE_F64LE, dataspace_id);
                this->check_dataset_space_1_dim (dataset_id, vals.size());
                status = H5Dwrite (dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(outvals[0]));

            } else if constexpr (std::is_same<std::decay_t<T>, float>::value == true) {
                dataset_id = this->open_dataset (path, H5T_IEEE_F64LE, dataspace_id);
                this->check_dataset_space_1_dim (dataset_id, vals.size());
                status = H5Dwrite (dataset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(outvals[0]));

            } else if constexpr (std::is_same<std::decay_t<T>, int>::value == true) {
                dataset_id = this->open_dataset (path, H5T_STD_I64LE, dataspace_id);
                this->check_dataset_space_1_dim (dataset_id, vals.size());
                status = H5Dwrite (dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(outvals[0]));

            } else if constexpr (std::is_same<std::decay_t<T>, long long int>::value == true) {
                dataset_id = this->open_dataset (path, H5T_STD_I64LE, dataspace_id);
                this->check_dataset_space_1_dim (dataset_id, vals.size());
                status = H5Dwrite (dataset_id, H5T_NATIVE_LLONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(outvals[0]));

            } else if constexpr (std::is_same<std::decay_t<T>, unsigned int>::value == true) {
                dataset_id = this->open_dataset (path, H5T_STD_U64LE, dataspace_id);
                this->check_dataset_space_1_dim (dataset_id, vals.size());
                status = H5Dwrite (dataset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(outvals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, unsigned long long int>::value == true) {
                dataset_id = this->open_dataset (path, H5T_STD_U64LE, dataspace_id);
                this->check_dataset_space_1_dim (dataset_id, vals.size());
                status = H5Dwrite (dataset_id, H5T_NATIVE_ULLONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(outvals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, cv::Point2i>::value == true) {
                dataset_id = this->open_dataset (path, H5T_STD_I64LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, vals.size(), 2);
                status = H5Dwrite (dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(outvals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, cv::Point2d>::value == true) {
                dataset_id = this->open_dataset (path, H5T_IEEE_F64LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, vals.size(), 2);
                status = H5Dwrite (dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(outvals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, cv::Point2f>::value == true) {
                dataset_id = this->open_dataset (path, H5T_IEEE_F64LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, vals.size(), 2);
                status = H5Dwrite (dataset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(outvals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, std::array<float,2>>::value == true) {
                dataset_id = this->open_dataset (path, H5T_IEEE_F64LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, vals.size(), 2);
                status = H5Dwrite (dataset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(outvals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, std::array<double,2>>::value == true) {
                dataset_id = this->open_dataset (path, H5T_IEEE_F64LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, vals.size(), 2);
                status = H5Dwrite (dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(outvals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, std::pair<float, float>>::value == true) {
                dataset_id = this->open_dataset (path, H5T_IEEE_F64LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, vals.size(), 2);
                status = H5Dwrite (dataset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(outvals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, std::pair<double, double>>::value == true) {
                dataset_id = this->open_dataset (path, H5T_IEEE_F64LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, vals.size(), 2);
                status = H5Dwrite (dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(outvals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, std::pair<int, int>>::value == true) {
                dataset_id = this->open_dataset (path, H5T_STD_I64LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, vals.size(), 2);
                status = H5Dwrite (dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(outvals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, std::pair<unsigned int, unsigned int>>::value == true) {
                dataset_id = this->open_dataset (path, H5T_STD_U64LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, vals.size(), 2);
                status = H5Dwrite (dataset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(outvals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, std::pair<long long int, long long int>>::value == true) {
                dataset_id = this->open_dataset (path, H5T_STD_I64LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, vals.size(), 2);
                status = H5Dwrite (dataset_id, H5T_NATIVE_LLONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(outvals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, std::pair<unsigned long long int, unsigned long long int>>::value == true) {
                dataset_id = this->open_dataset (path, H5T_STD_U64LE, dataspace_id);
                this->check_dataset_space_2_dims (dataset_id, vals.size(), 2);
                status = H5Dwrite (dataset_id, H5T_NATIVE_ULLONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(outvals[0]));

            } else {
                throw std::runtime_error ("HdfData::add_contained_vals<Container<T, Allocator>>: Don't know how to store that type");
            }
            this->handle_error (status, "Error. status after H5Dwrite 4: ");
            status = H5Dclose (dataset_id);
            this->handle_error (status, "Error. status after H5Dclose: ");
            status = H5Sclose (dataspace_id);
            this->handle_error (status, "Error. status after H5Sclose: ");
        }

        /*!
         * Templated version of read_contained_vals, for vector/list/deque (but not map,
         * which is more complex) and whatever simple value (int, double, float, etc) is
         * contained. Use this to read, for example: std::vector<double> or
         * std::deque<unsigned int> or std::vector<float>. Can *also* do
         * std::vector<cv::Point> or std::deque<cv::Point2d> etc. Unlike plain
         * read_contained_vals in HdfData.
         */
        template < template <typename, typename> typename Container,
                   typename T,
                   typename Allocator=std::allocator<T> >
        void read_contained_vals (const char* path, Container<T, Allocator>& vals)
        {
            hid_t dataset_id = H5Dopen2 (this->file_id, path, H5P_DEFAULT);
            if (this->check_dataset_id (dataset_id, path) == -1) { return; }

            hid_t space_id = H5Dget_space (dataset_id);
            // Regardless of read_error_action, throw exception here; missing paths
            // should cause missing dataset_id
            if (space_id < 0) {
                std::stringstream ee;
                ee << "Error: Failed to get a dataset space_id for dataset_id " << dataset_id;
                throw std::runtime_error (ee.str());
            }

            // Read the data from HDF5 into a vector. Thereafter, copy it into the
            // Container vals. This ensures vals can be std::list.
            std::vector<T> invals;

            // If cv::Point like. Could add pair<float, float> and pair<double, double>,
            // also container of array<T, 2> also.
            if constexpr (std::is_same<std::decay_t<T>, cv::Point2i>::value == true
                          || std::is_same<std::decay_t<T>, cv::Point2f>::value == true
                          || std::is_same<std::decay_t<T>, cv::Point2d>::value == true
                          || std::is_same<std::decay_t<T>, std::array<float, 2>>::value == true
                          || std::is_same<std::decay_t<T>, std::array<double, 2>>::value == true
                          || std::is_same<std::decay_t<T>, std::pair<float, float>>::value == true
                          || std::is_same<std::decay_t<T>, std::pair<double, double>>::value == true
                          || std::is_same<std::decay_t<T>, std::pair<int, int>>::value == true
                          || std::is_same<std::decay_t<T>, std::pair<unsigned int, unsigned int>>::value == true
                          || std::is_same<std::decay_t<T>, std::pair<long long int, long long int>>::value == true
                          || std::is_same<std::decay_t<T>, std::pair<unsigned long long int, unsigned long long int>>::value == true) {
                hsize_t dims[2] = {0,0}; // 2D
                int ndims = H5Sget_simple_extent_dims (space_id, dims, NULL);
                if (ndims != 2) {
                    std::stringstream ee;
                    ee << "In:\n" << __PRETTY_FUNCTION__
                       << "\nError: Expected 2D data to be stored in " << path << ". ndims=" << ndims;
                    throw std::runtime_error (ee.str());
                }
                if (dims[1] != 2) {
                    std::stringstream ee;
                    ee << "In:\n" << __PRETTY_FUNCTION__
                       << ":\nError: Expected 2 coordinates to be stored in each cv::Point/array<*,2>/pair<> of " << path;
                    throw std::runtime_error (ee.str());
                }
                invals.resize (dims[0]);
                vals.resize (dims[0]);

            } else {
                // If standard thing like double, float, int etc:
                hsize_t dims[1] = {0}; // 1D

                int ndims = H5Sget_simple_extent_dims (space_id, dims, NULL);
                if (ndims != 1) {
                    std::stringstream ee;
                    ee << "In:\n" << __PRETTY_FUNCTION__
                       << ":\nError: Expected 1D data to be stored in " << path << ". ndims=" << ndims;
                    throw std::runtime_error (ee.str());
                }
                invals.resize (dims[0], T{0});
                vals.resize (dims[0], T{0});
            }

            herr_t status = 0;
            if constexpr (std::is_same<std::decay_t<T>, double>::value == true) {
                status = H5Dread (dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(invals[0]));

            } else if constexpr (std::is_same<std::decay_t<T>, float>::value == true) {
                status = H5Dread (dataset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(invals[0]));

            } else if constexpr (std::is_same<std::decay_t<T>, int>::value == true) {
                status = H5Dread (dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(invals[0]));

            } else if constexpr (std::is_same<std::decay_t<T>, long long int>::value == true) {
                status = H5Dread (dataset_id, H5T_NATIVE_LLONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(invals[0]));

            } else if constexpr (std::is_same<std::decay_t<T>, unsigned int>::value == true) {
                status = H5Dread (dataset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(invals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, unsigned long long int>::value == true) {
                status = H5Dread (dataset_id, H5T_NATIVE_ULLONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(invals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, cv::Point2i>::value == true) {
                status = H5Dread (dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(invals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, cv::Point2d>::value == true) {
                status = H5Dread (dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(invals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, cv::Point2f>::value == true) {
                status = H5Dread (dataset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(invals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, std::array<float,2>>::value == true) {
                status = H5Dread (dataset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(invals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, std::array<double,2>>::value == true) {
                status = H5Dread (dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(invals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, std::pair<float, float>>::value == true) {
                status = H5Dread (dataset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(invals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, std::pair<double, double>>::value == true) {
                status = H5Dread (dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(invals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, std::pair<int, int>>::value == true) {
                status = H5Dread (dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(invals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, std::pair<unsigned int, unsigned int>>::value == true) {
                status = H5Dread (dataset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(invals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, std::pair<long long int, long long int>>::value == true) {
                status = H5Dread (dataset_id, H5T_NATIVE_LLONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(invals[0]));

            } else if constexpr (std::is_same<typename std::decay<T>::type, std::pair<unsigned long long int, unsigned long long int>>::value == true) {
                status = H5Dread (dataset_id, H5T_NATIVE_ULLONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(invals[0]));

            } else {
                throw std::runtime_error ("HdfData::read_contained_vals<T>: Don't know how to read that type");
            }

            // Copy invals into vals
            std::copy (invals.begin(), invals.end(), vals.begin());

            this->handle_error (status, "Error. status after H5Dread: ");
            status = H5Dclose (dataset_id);
            this->handle_error (status, "Error. status after H5Dclose: ");
        }

        /*!
         * Read an OpenCV Matrix that was stored with the sister add_contained_vals
         * function (which also stores some necessary metadata).
         */
        void read_contained_vals (const char* path, cv::Mat& vals)
        {
            // First get the type metadata
            std::string pathtype (path);
            pathtype += "_type";
            int cv_type = 0;
            this->read_val (pathtype.c_str(), cv_type);
            pathtype = std::string(path) + "_channels";
            int channels = 0;
            this->read_val (pathtype.c_str(), channels);

            // Now read the matrix
            hid_t dataset_id = H5Dopen2 (this->file_id, path, H5P_DEFAULT);
            if (this->check_dataset_id (dataset_id, path) == -1) { return; }
            hid_t space_id = H5Dget_space (dataset_id);
            hsize_t dims[2] = {0,0};
            int ndims = H5Sget_simple_extent_dims (space_id, dims, NULL);
            if (ndims != 2) {
                std::stringstream ee;
                ee << "Error. Expected 2D data to be stored in " << path;
                throw std::runtime_error (ee.str());
            }

            // Dims gives size in absolute number of elements. If channels > 1, then the Mat
            // needs to be resized accordingly
            int matcols = dims[1] / channels;

            // Resize the Mat
            vals.create ((int)dims[0], matcols, cv_type);

            herr_t status;
            switch (cv_type) {
            case CV_8UC1:
            case CV_8UC2:
            case CV_8UC3:
            case CV_8UC4:
            {
                status = H5Dread (dataset_id, H5T_NATIVE_UCHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT, vals.data);
                break;
            }
            case CV_8SC1:
            case CV_8SC2:
            case CV_8SC3:
            case CV_8SC4:
            {
                status = H5Dread (dataset_id, H5T_NATIVE_CHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT, vals.data);
                break;
            }
            case CV_16UC1:
            case CV_16UC2:
            case CV_16UC3:
            case CV_16UC4:
            {
                status = H5Dread (dataset_id, H5T_NATIVE_USHORT, H5S_ALL, H5S_ALL, H5P_DEFAULT, vals.data);
                break;
            }
            case CV_16SC1:
            case CV_16SC2:
            case CV_16SC3:
            case CV_16SC4:
            {
                status = H5Dread (dataset_id, H5T_NATIVE_SHORT, H5S_ALL, H5S_ALL, H5P_DEFAULT, vals.data);
                break;
            }
            case CV_32SC1:
            case CV_32SC2:
            case CV_32SC3:
            case CV_32SC4:
            {
                status = H5Dread (dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, vals.data);
                break;
            }
            case CV_32FC1:
            case CV_32FC2:
            case CV_32FC3:
            case CV_32FC4:
            {
                status = H5Dread (dataset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, vals.data);
                break;
            }
            case CV_64FC1:
            case CV_64FC2:
            case CV_64FC3:
            case CV_64FC4:
            {
                status = H5Dread (dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, vals.data);
                break;
            }
            default:
            {
                //cerr << "Unknown CvType " << cv_type << endl;
                status = -1; // What's correct for an error here?
                break;
            }
            }
            this->handle_error (status, "Error. status after H5Dread: ");
            status = H5Dclose (dataset_id);
            this->handle_error (status, "Error. status after H5Dclose: ");
        }

    };
}