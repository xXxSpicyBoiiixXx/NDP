#pragma once

typedef int32_t ndp_uffd_t;

ndp_uffd_t ndp_uffd_create();
void ndp_uffd_setup(ndp_uffd_t uffd);
void ndp_uffd_register_mem(ndp_uffd_t uffd, struct ndp_mapped_buf_t buf);
void ndp_uffd_copy(ndp_uffd_t uffd, void *dest, void *src, size_t len);
