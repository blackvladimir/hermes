#cmakedefine WITH_UMFPACK
#cmakedefine WITH_PARDISO
#cmakedefine WITH_MUMPS
#cmakedefine WITH_PETSC
#cmakedefine WITH_HDF5
#cmakedefine WITH_EXODUSII
#cmakedefine WITH_MPI

// stacktrace
#cmakedefine HAVE_TEUCHOS_LINK
#cmakedefine HAVE_TEUCHOS_BFD
#cmakedefine HAVE_EXECINFO
#cmakedefine HAVE_VASPRINTF
#cmakedefine HAVE_CXXABI

// trilinos
#cmakedefine WITH_TRILINOS
#cmakedefine HAVE_AMESOS
#cmakedefine HAVE_AZTECOO
#cmakedefine HAVE_TEUCHOS
#cmakedefine HAVE_EPETRA
#cmakedefine HAVE_IFPACK
#cmakedefine HAVE_ML
#cmakedefine HAVE_NOX
#cmakedefine HAVE_KOMPLEX

#cmakedefine TRACING
#cmakedefine DEBUG
#cmakedefine DEBUG_ORDER

// elements
#cmakedefine WITH_TETRA
#cmakedefine WITH_HEX
#cmakedefine WITH_PRISM

#cmakedefine PRELOADING

/* --- */
#define PACKAGE_BUGREPORT "dandrs@unr.edu"
#define PACKAGE_NAME "Hermes3D"
#define PACKAGE_TARNAME "hermes3d"
#define PACKAGE_VERSION "0.0.1"
#define PACKAGE_STRING "Hermes3D v" PACKAGE_VERSION