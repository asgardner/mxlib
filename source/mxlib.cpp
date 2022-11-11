/** \file mxlib.cpp
 * \author Jared R. Males
 * \brief Implementations of some libarary wide utilities
 *
 */

//***********************************************************************//
// Copyright 2021 Jared R. Males (jaredmales@gmail.com)
//
// This file is part of mxlib.
//
// mxlib is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// mxlib is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with mxlib.  If not, see <http://www.gnu.org/licenses/>.
//***********************************************************************//

namespace mx
{

    const char *mxlib_comp_current_branch()
    {
        return "";
    }

    const char *mxlib_comp_current_sha1()
    {
        return "";
    }

    bool mxlib_comp_repo_modified()
    {
        return true;
    }

} // namespace mx
