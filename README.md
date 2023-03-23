<!--
*** @ref: https://github.com/othneildrew/Best-README-Template
-->

<!-- PROJECT HEADER -->
<br />
<div align="center">
  <a href="https://github.com/Siarh199/external_sorter">
  </a>

<h3 align="center">external_sorter</h3>

  <p align="center">
    The library for performing external sorting (https://en.wikipedia.org/wiki/External_sorting) of binary files with
numbers to an output directory.
  </p>
</div>



<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
      <ul>
        <li><a href="#built-with">Built With</a></li>
      </ul>
    </li>
    <li><a href="#algorithm">Algorithm</a></li>
    <li><a href="#contributing">Contributing</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#contact">Contact</a></li>
  </ol>
</details>



<!-- ABOUT THE PROJECT -->

## About The Project

external_sorter library provides possibility to sort 'big' binary files of numbers (std::uint32_t by default) using
multithreading approach.

Main classes:

* `ThreadPool` class contains a pool of threads for performing tasks (`std::function`) in parallel.
* `ExternalSorter` class is the main class which performs external sorting.
* `BinaryFileBuffer` class serves for paralleling file operations. It allows to read/write the first part of the buffer.
* `ThreadSafeQueue` class serves for managing buffers while reading/sorting/writing chunks of an input file.

### Built With

The project does not have any external dependencies (except gtest for unit tests) and should be compilable with any C++
compiler which supports C++17 (including std::filesystem).

Unit tests can be disabled by option `ENABLE_TESTING` (enabled by default).

Console application can be disabled by option `ENABLE_CONSOLE_APP` (enabled by default).

Cmake v3.10 or higher is required.



<!-- ALGORITHM -->

## Algorithm

Class `ExternalSorter` performs external sorting logic in the following way:

* Create an intermediate directory for sorted chunks `ExternalSorter::createIntermediateDirectory()`.
* Read an input file and create intermediate sorted chunks in the intermediate
  directory `ExternalSorter::createSortedChunksImplMultiThreaded()`:
    1. Create thread-safe queue of buffers for numbers.
    2. Allocate buffers for numbers with `size = available_memory / threads_count`.
    3. Read the input file to buffers from the queue and then sort buffers in other threads.
    4. Write sorted buffers to the intermediate directory and return buffers to the queue.
* Merge sorted chunks to an output file `ExternalSorter::mergeSortedChunksImpl()`:
    1. Determine size of buffers for reading (using preload mechanism in other thread) sorted chunks and create them.
    2. Create two buffers for merging. The first one is used for merging while the second one used for writing merging
       results to a disk.
    3. Merge sorted chunks (via the buffers for reading) using min heap in a buffer for merging while writing another
       merged buffer to output file in a separate thread.

NOTE: It is necessary to specify reasonable amount of available memory (>1mb) in `ExternalSorter` constructor.



<!-- CONTRIBUTING -->

## Contributing

Contributions are what make the open source community such an amazing place to learn, inspire, and create. Any
contributions you make are **greatly appreciated**.

If you have a suggestion that would make this better, please fork the repo and create a pull request. You can also
simply open an issue with the tag "enhancement".
Don't forget to give the project a star! Thanks again!

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

<!-- LICENSE -->

## License

Distributed under the MIT License. See `LICENSE.txt` for more information.



<!-- CONTACT -->

## Contact

Sergey Gorelyshev - s.gorelyshev90@gmail.com

Project Link: [https://github.com/Siarh199/external_sorter](https://github.com/Siarh199/external_sorter)
