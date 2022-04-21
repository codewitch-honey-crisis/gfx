for %%f in (%1) do (
    @echo Generating %%~nf.hpp
    fontgen "%%f" > "%%~nf.hpp"
)